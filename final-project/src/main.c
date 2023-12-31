#include "clock-server.h"
#include "console-server.h"
#include "marklin-server.h"
#include "nameserver.h"
#include "msg.h"
#include "task.h"
#include "time.h"
#include "uart-server.h"
#include "uassert.h"

#include "control-msgs.h"
#include "clock.h"
#include "monitor.h"
#include "program-tasks.h"
#include "speed-data.h"
#include "snake.h"
#include "track.h"
#include "track-data.h"
#include "user.h"

speed_data spd_data;
track_node track[TRACK_MAX];

static void init_track_data(uint16_t console) {
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

    speed_data_init(&spd_data);
    init_track_data(console_tid);
    init_monitor(console_tid);

    init_track(marklin_tid);
    WaitOutputEmpty(marklin_tid);

    Create(P_HIGH, time_task_main);
    Create(P_LOW, idle_time_task_main);
    Create(P_MED, user_server_main);

    // start snake
    uint16_t snake_tid = Create(P_HIGH, snake_server_main);

    BlockUntilReady();

    // quitting
    BlockUntilQuit();

    // need to manually shut down snake
    snake_server_end(snake_tid);

    // shutdown all non-server children to exit gracefully
    KillAllChildren();

    // send shutdown commands
    shutdown_monitor(console_tid);
    shutdown_track(marklin_tid);
}
