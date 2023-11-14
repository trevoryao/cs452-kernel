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

speed_data spd_data;
track_node track[TRACK_MAX];

void client1_test(void) {
    int consoleTid = WhoIs(CONSOLE_SERVER_NAME);
    int tsTid = WhoIs(TS_SERVER_NAME);

    uart_printf(CONSOLE, "Client 1 - Woke up\r\n");
    track_server_acquire_server_lock(tsTid);

    uart_printf(CONSOLE, "Client 1 - Acquired server lock\r\n");
    // locking segment 21
    bool lock1 = track_server_lock_segment(tsTid, &track[53], DIR_AHEAD);
    uart_printf(CONSOLE, "Client 1 - Locked segment 21: %d\r\n", lock1);

    bool lock2 = track_server_lock_segment(tsTid, &track[94], DIR_STRAIGHT);
    uart_printf(CONSOLE, "Client 1 - Locked segment 12: %d\r\n", lock2);
    
    bool lock3 = track_server_lock_segment(tsTid, &track[102], DIR_STRAIGHT);
    uart_printf(CONSOLE, "Client 1 - Locked segment 30: %d\r\n", lock3);

    track_server_free_segment(tsTid, &track[53]);
    uart_printf(CONSOLE, "Client 1 - Freed segment 21\r\n");

    track_server_free_server_lock(tsTid);
    uart_printf(CONSOLE, "Client 1 - done\r\n");

}

void client2_test(void) {
    int consoleTid = WhoIs(CONSOLE_SERVER_NAME);
    int tsTid = WhoIs(TS_SERVER_NAME);

    uart_printf(CONSOLE, "Client 2 - Woke up\r\n");
    track_server_acquire_server_lock(tsTid);

    uart_printf(CONSOLE, "Client 2 - Acquired server lock\r\n");
    // locking segment 21
    bool lock2 = track_server_lock_segment(tsTid, &track[96], DIR_CURVED);
    uart_printf(CONSOLE, "Client 2 - trying to lock segment 12 in wrong direction - should fail: %d\r\n", lock2);

    // locking segment 21
    bool lock3 = track_server_lock_segment(tsTid, &track[94], DIR_STRAIGHT);
    uart_printf(CONSOLE, "Client 2 - trying to lock segment 12 in right direction - should work: %d\r\n", lock3);


    bool lock4 = track_server_lock_segment(tsTid, &track[53], DIR_AHEAD);
    uart_printf(CONSOLE, "Client 2 - trying to lock segment 21 in right direction - should work: %d\r\n", lock4);

    bool lock5 = track_server_lock_segment(tsTid, &track[102], DIR_STRAIGHT);
    uart_printf(CONSOLE, "Client 2 - Trying to lock one only segment 30 - should fail: %d\r\n", lock5);

    track_server_free_server_lock(tsTid);
    uart_printf(CONSOLE, "Client 2 - done\r\n");

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
