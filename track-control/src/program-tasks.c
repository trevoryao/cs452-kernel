#include "program-tasks.h"

#include <stdint.h>

#include "clock.h"
#include "deque.h"
#include "nameserver.h"
#include "msg.h"
#include "task.h"
#include "time.h"
#include "uart-server.h"
#include "uassert.h"

#include "control-msgs.h"
#include "monitor.h"
#include "parsing.h"
#include "sensors.h"
#include "speed.h"
#include "track.h"
#include "track-control.h"
#include "track-control-coordinator.h"
#include "train.h"

// pass to worker task to prevent perf hit of multiple syscalls
typedef struct msg_rv {
    uint16_t clock_tid;
    uint16_t tcc_tid;
    uint16_t spd;
    uint16_t trn;
    uint32_t delay_until_time;
} msg_rv;

void time_task_main(void) {
    uint16_t clock_tid = WhoIs(CLOCK_SERVER_NAME);
    uint16_t console_tid = WhoIs(CONSOLE_SERVER_NAME);

    uint32_t real_time = Time(clock_tid);
    uint32_t target = real_time;

    time_t time;

    for (;;) {
        time_from_clock_ticks(&time, real_time);
        update_time(console_tid, &time);

        target += REFRESH_TIME;
        real_time = DelayUntil(clock_tid, target);
    }
}

void idle_time_task_main(void) {
    uint16_t clock_tid = WhoIs(CLOCK_SERVER_NAME);
    uint16_t console_tid = WhoIs(CONSOLE_SERVER_NAME);

    uint32_t real_time = Time(clock_tid);
    uint32_t target = real_time;

    uint64_t idle_sys_ticks, user_sys_ticks;

    for (;;) {
        GetIdleStatus(&idle_sys_ticks, &user_sys_ticks);
        update_idle(console_tid, idle_sys_ticks, user_sys_ticks);

        target += IDLE_REFRESH_TIME;
        real_time = DelayUntil(clock_tid, target);
    }
}

static void reverse_task_main(void) {
    msg_rv params;
    int ptid;
    uassert(Receive(&ptid, (char *)&params, sizeof(msg_rv)) == sizeof(msg_rv));
    Reply(ptid, NULL, 0);

    DelayUntil(params.clock_tid, params.delay_until_time);

    track_control_set_train_speed(params.tcc_tid, params.trn, SP_REVERSE);
    track_control_set_train_speed(params.tcc_tid, params.trn, params.spd);
}

void cmd_task_main(void) {
    uint16_t clock_tid = WhoIs(CLOCK_SERVER_NAME);
    uint16_t console_tid = WhoIs(CONSOLE_SERVER_NAME);
    uint16_t marklin_tid = WhoIs(MARKLIN_SERVER_NAME);
    uint16_t tcc_tid = WhoIs(TC_SERVER_NAME);

    deque console_in; // entered command deque
    deque_init(&console_in, 10);

    char c;
    cmd_s cmd; // for parsing

    for (;;) {
        c = Getc(console_tid);

        if (c == '\b' && !deque_empty(&console_in)) { // backspace
            deque_pop_back(&console_in);
            cmd_delete(console_tid); // delete last from console
        } else if (c == '\r') { // enter
            parse_cmd(&console_in, &cmd);
            deque_reset(&console_in);
            reset_error_message(console_tid);

            switch (cmd.kind) {
                case CMD_TR:
                    track_control_set_train_speed(tcc_tid, cmd.params[0], cmd.params[1]);
                    break;
                case CMD_RV:
                    track_control_set_train_speed(tcc_tid, cmd.params[0], SP_STOP);
                    int rev_time = Time(clock_tid) + RV_WAIT_TIME;

                    int rev_tid = Create(P_HIGH, reverse_task_main);
                    msg_rv params = {
                        clock_tid,
                        tcc_tid,
                        track_control_get_train_speed(tcc_tid, cmd.params[0]),
                        cmd.params[0],
                        rev_time
                    };
                    Send(rev_tid, (char *)&params, sizeof(msg_rv), NULL, 0);
                    break;
                case CMD_SW:
                    track_control_set_switch(tcc_tid, cmd.params[0], (enum SWITCH_DIR)cmd.params[1]);
                    break;
                case CMD_TC:
                    CreateControlledTrain(
                        cmd.params[1],
                        cmd.params[2],
                        cmd.path[0],
                        cmd.path[1],
                        cmd.params[0]
                    );
                    break;
                case CMD_GO:
                    track_go(marklin_tid);
                    break;
                case CMD_HLT:
                    track_stop(marklin_tid);
                    break;
                case CMD_Q: goto ProgramEnd;
                default:
                    print_error_message(console_tid);
                    break; // error (do nothing)
            }

            print_prompt(console_tid); // clear last cmd
        } else if (32 <= c && c <= 126) { // alphanum-sym
            deque_push_back(&console_in, c);
            cmd_out(console_tid, c); // echo out to console
        }
    }

ProgramEnd:;
    uint16_t ptid = MyParentTid();
    msg_control msg = {MSG_CONTROL_QUIT};
    uassert(Send(ptid, (char *)&msg, sizeof(msg_control), (char *)&msg, sizeof(msg_control)) == sizeof(msg_control));
    uassert(msg.type == MSG_CONTROL_ACK);
}
