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
    // WaitOutputEmpty(marklin_tid);

    // fake snake
    snake snake;
    snake_init(&snake);
    snake.head = 2;
    snake.trns[0].trn = 24;
    snake.trns[1].trn = 77;
    snake.trns[2].trn = 58;

    update_snake(console_tid, &snake);
    update_snake_distance(console_tid, &snake, 23, 1, 239);
    update_snake_distance(console_tid, &snake, 23, 1, 10);

    update_next_input_switch(console_tid, 155);

    for (;;) {
        Yield();
    }
}
