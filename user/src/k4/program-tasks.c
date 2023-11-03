#include "k4/program-tasks.h"

#include <stdint.h>

#include "clock.h"
#include "deque.h"
#include "nameserver.h"
#include "msg.h"
#include "task.h"
#include "time.h"
#include "uart-server.h"
#include "uassert.h"

#include "k4/control-msgs.h"
#include "k4/monitor.h"
#include "k4/parsing.h"
#include "k4/sensors.h"
#include "k4/speed.h"
#include "k4/track.h"

// pass to worker task to prevent perf hit of multiple syscalls
typedef struct msg_rv {
    uint16_t clock_tid;
    uint16_t marklin_tid;
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

static void reverse_task_main(void) {
    msg_rv params;
    int ptid;
    uassert(Receive(&ptid, (char *)&params, sizeof(msg_rv)) == sizeof(msg_rv));
    Reply(ptid, NULL, 0);

    DelayUntil(params.clock_tid, params.delay_until_time);
    train_reverse_end(params.marklin_tid, params.spd, params.trn);
}

void cmd_task_main(void) {
    uint16_t clock_tid = WhoIs(CLOCK_SERVER_NAME);
    uint16_t console_tid = WhoIs(CONSOLE_SERVER_NAME);
    uint16_t marklin_tid = WhoIs(MARKLIN_SERVER_NAME);

    deque console_in; // entered command deque
    deque_init(&console_in, 10);

    char c;
    cmd_s cmd; // for parsing

    speed_t spd_t;
    speed_t_init(&spd_t);

    for (;;) {
        c = Getc(console_tid);

        if (c == '\b' && !deque_empty(&console_in)) { // backspace
            deque_pop_back(&console_in);
            cmd_delete(console_tid); // delete last from console
        } else if (c == '\r') { // enter
            parse_cmd(&console_in, &cmd);
            deque_reset(&console_in);

            switch (cmd.kind) {
                case CMD_TR:
                    train_mod_speed(marklin_tid, &spd_t, cmd.argv[0], cmd.argv[1]);
                    update_speed(console_tid, &spd_t, cmd.argv[0]);
                    break;
                case CMD_RV:
                    train_reverse_start(marklin_tid, &spd_t, cmd.argv[0]);
                    int rev_time = Time(clock_tid) + RV_WAIT_TIME;

                    int rev_tid = Create(P_HIGH, reverse_task_main);
                    msg_rv params = {
                        clock_tid,
                        marklin_tid,
                        speed_get(&spd_t, cmd.argv[0]),
                        cmd.argv[0],
                        rev_time
                    };
                    Send(rev_tid, (char *)&params, sizeof(msg_rv), NULL, 0);

                    update_speed(console_tid, &spd_t, cmd.argv[0]);
                    break;
                case CMD_SW:
                    switch_throw(marklin_tid, cmd.argv[0], (enum SWITCH_DIR)cmd.argv[1]);
                    update_switch(console_tid, cmd.argv[0], (enum SWITCH_DIR)cmd.argv[1]);
                    break;
                case CMD_GO:
                    track_go(marklin_tid);
                    break;
                case CMD_HLT:
                    track_stop(marklin_tid);
                    break;
                case CMD_Q: goto ProgramEnd;
                default: break; // error (do nothing)
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

void sensor_task_main(void) {
    uint16_t console_tid = WhoIs(CONSOLE_SERVER_NAME);
    uint16_t marklin_tid = WhoIs(MARKLIN_SERVER_NAME);

    struct sensor sensor;
    sen_data_init(&sensor);

    deque triggered_sensors;
    deque_init(&triggered_sensors, 4); // 16 (8 in total)

    char sen_byte;

    for (;;) {
        WaitOutputEmpty(marklin_tid); // make sure no commands to send

        sen_start_dump(marklin_tid); // queues sending commands
        for (;;) {
            sen_byte = Getc(marklin_tid);

            if (rcv_sen_dump(&sensor, sen_byte, console_tid, &triggered_sensors)) // finished?
                break; // inner for loop
        }
    }
}
