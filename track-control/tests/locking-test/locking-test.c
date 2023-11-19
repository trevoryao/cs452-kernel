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

    deque_push_back(&segments, 5);
    deque_push_back(&segments, 4);
    deque_push_back(&segments, 3);

    uart_printf(CONSOLE, "Client 1 - Woke up\r\n");

    bool success = track_server_lock_all_segments_timeout(tsTid, &segments, trainNo, 20);
    uart_printf(CONSOLE, "Client 1 - locked all segments: %d\r\n", success);

    Delay(clock, 100);

    uart_printf(CONSOLE, "Client 1 - freeing the segments\r\n");
    track_server_free_segments(tsTid, &segments, trainNo);
    uart_printf(CONSOLE, "Client 1 - freed the segments\r\n");

    Delay(clock, 100);

    // try to lock one 
    int segment = track_server_lock_one_segment_timeout(tsTid, &segments, trainNo, 100);
    uart_printf(CONSOLE, "Client 1 - locked one segments: %d\r\n", segment);

    // check which segments are locked
    for (int i = 0; i < 30; i++){
        uart_printf(CONSOLE, "segment %d is locked : %d\r\n", i, track_server_segment_is_locked(tsTid, i, trainNo));
    }
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

    deque second_segments;
    deque_init(&second_segments, 4);
    deque_push_back(&second_segments, 10);
    deque_push_back(&second_segments, 9);
    deque_push_back(&second_segments, 8);

    Delay(clock, 2);
    uart_printf(CONSOLE, "Client 2 - Woke up\r\n");

    uart_printf(CONSOLE, "Client 2 -  trying to timeout lock\r\n");
    // trying to lock all with timeout
    int lock = track_server_lock_two_all_segments_timeout(tsTid, &segments, &second_segments, trainNo, 20);
    uart_printf(CONSOLE, "Client 2 - locked all segments timeout - should work: %d\r\n", lock);

    track_server_lock_all_segments(tsTid, &segments, trainNo);
    uart_printf(CONSOLE, "Client 2 - locked all segments\r\n");

    uart_printf(CONSOLE, "Client 2 - Done\r\n");
}

void user_main(void) {
    // start up clock, uart servers
    int clock = Create(P_SERVER_HI, clockserver_main);
    Create(P_SERVER_LO, console_server_main);

    uart_printf(CONSOLE, "Test started\r\n");

    
    Create(P_SERVER_HI, track_server_main);

    Delay(clock, 100);

    uart_printf(CONSOLE, "Track server started\r\n");

    // reset track to get our loop
    speed_data_init(&spd_data);
    init_track_b(track);

    Create(P_HIGH, client1_test);
    Create(P_HIGH, client2_test);

    for(;;) {
        // some stupid loop

        Delay(clock, 1000);
    }
}
