#include <stdbool.h>

#include "uart-server.h"
#include "uassert.h"
#include "marklin-server.h"
#include "console-server.h"
#include "task.h"
#include "clock-server.h"
#include "clock.h"
#include "term-control.h"
#include "time.h"
#include "sys-clock.h"

#include "track.h"
#include "speed-data.h"
#include "speed.h"
#include "track-data.h"
#include "track-node.h"
#include "rpi.h"

#include "track-segment-locking.h"
#include "track-server.h"
#include "nameserver.h"
#include "deque.h"

speed_data spd_data;
track_node track[TRACK_MAX];

void client1_test(void) {
    int consoleTid = WhoIs(CONSOLE_SERVER_NAME);
    int tsTid = WhoIs(TS_SERVER_NAME);
    int clock = WhoIs(CLOCK_SERVER_NAME);

    int trainNo = 77;

    deque segments;
    deque_init(&segments, 4);

    deque_push_back(&segments, 3);
    deque_push_back(&segments, 4);
    deque_push_back(&segments, 5);

    uart_printf(CONSOLE, "Client 1 - Woke up\r\n");

    bool success = track_server_lock_all_segments_timeout(tsTid, &segments, trainNo, 20);
    uart_printf(CONSOLE, "Client 1 - locked all segments: %d", success);

    Delay(clock, 100);

    track_server_free_segments(tsTid, &segments, trainNo);

    Delay(clock, 100);

    // try to lock one 
    int segment = track_server_lock_one_segments_timeout(tsTid, &segments, trainNo, 100);
    uart_printf(CONSOLE, "Client 1 - locked one segments: %d", segment);


}

void client2_test(void) {
    int consoleTid = WhoIs(CONSOLE_SERVER_NAME);
    int tsTid = WhoIs(TS_SERVER_NAME);
    int clock = WhoIs(CLOCK_SERVER_NAME);


    int trainNo = 24;

    deque segments;
    deque_init(&segments, 4);
    deque_push_back(&segments, 3);
    deque_push_back(&segments, 4);
    deque_push_back(&segments, 6);

    Delay(clock, 2);
    uart_printf(CONSOLE, "Client 2 - Woke up\r\n");

    // trying to lock all with timeout
    bool success = track_server_lock_all_segments_timeout(tsTid, &segments, trainNo, 20);
    uart_printf(CONSOLE, "Client 2 - locked all segments timeout - should fail: %d", success);


    track_server_lock_all_segments(tsTid, &segments, trainNo);
    uart_printf(CONSOLE, "Client 2 - locked all segments");

    uart_printf(CONSOLE, "Client 2 - Done\r\n");
}

void user_main(void) {
    // start up clock, uart servers
    Create(P_SERVER_HI, clockserver_main);
    Create(P_SERVER_LO, console_server_main);
    
    Create(P_SERVER_HI, track_server_main);

    // reset track to get our loop
    speed_data_init(&spd_data);
    init_track_b(track);

    Create(P_HIGH, client1_test);
    Create(P_HIGH, client2_test);
}
