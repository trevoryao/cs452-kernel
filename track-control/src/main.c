#include "monitor.h"

#include "clock-server.h"
#include "console-server.h"
#include "marklin-server.h"
#include "nameserver.h"
#include "msg.h"
#include "task.h"
#include "time.h"
#include "uart-server.h"
#include "uassert.h"
#include "track-control-coordinator.h"

#include "control-msgs.h"
#include "program-tasks.h"
#include "speed-data.h"
#include "track.h"
#include "track-data.h"

speed_data spd_data;
track_node track[TRACK_MAX];

void init_track_data(uint16_t console) {
    for (;;) {
        Printf(console, "\r\nPlease Enter Track (A/B): ");
        char c = Getc(console);
        Printf(console, "%c\r\n", c);

        switch (c) {
            case 'a':
            case 'A':
                init_track_a(track);
                return;
            case 'b':
            case 'B':
                init_track_b(track);
                return;
            default:
                Printf(console, "Invalid Track!\r\n");
        }
    }
}

void user_main(void) {
    // start up clock, uart servers
    Create(P_SERVER_HI, clockserver_main);

    uint16_t console_tid = Create(P_SERVER_LO, console_server_main);
    uint16_t marklin_tid = Create(P_SERVER_HI, marklin_server_main);
    uint16_t tcc_tid = Create(P_SERVER_HI, track_control_coordinator_main);

    // initialisation commands
    speed_data_init(&spd_data);
    init_track_data(console_tid);
    init_monitor(console_tid);
    init_track(marklin_tid);

    // start all tasks
    Create(P_HIGH, time_task_main);
    Create(P_MED, cmd_task_main);
    //Create(P_HIGH, sensor_task_main);

    // wait for quit control msg from cmd_task
    int cmd_task_tid;
    msg_control msg;
    uassert(Receive(&cmd_task_tid, (char *)&msg, sizeof(msg_control)) == sizeof(msg_control));
    msg.type = MSG_CONTROL_ACK;
    uassert(Reply(cmd_task_tid, (char *)&msg, sizeof(msg_control)) == sizeof(msg_control));

    // shutdown all non-server children to exit gracefully
    KillAllChildren();

    // send shutdown commands
    shutdown_monitor(console_tid);
    shutdown_track(marklin_tid);
}
