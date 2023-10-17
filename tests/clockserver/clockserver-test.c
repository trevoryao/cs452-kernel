#include "sys-clock.h"
#include "rpi.h"
#include "uassert.h"
#include "nameserver.h"
#include "clock.h"
#include "clock-server.h"

#define N_TESTS 1500 // 15 seconds

// just testing that we can indeed run interrupts
void user_main(void) {
    Create(P_SERVER, clockserver_main);
    int clockserver_tid = WhoIs("clockserver");

    // Test getting the time
    int time = Time(clockserver_tid);
    uart_printf(CONSOLE, "Received clock-tick: %d \r\n", time);


    // test to delay for 10 ticks
    int time_before = get_curr_ticks() / TIMER_TICK_UNIT;
    int time_rec = Delay(clockserver_tid, 10);
    int time_after = get_curr_ticks() / TIMER_TICK_UNIT;
    uart_printf(CONSOLE, "Time before %d, Time after %d, Time rec %d\r\n", time_before, time_after, time_rec);

    uassert(time_after >= (time_before+10));

    // test the delay until
    time = Time(clockserver_tid);
    time_before = get_curr_ticks() / TIMER_TICK_UNIT;
    time_rec = DelayUntil(clockserver_tid, time+10);
    time_after = get_curr_ticks() / TIMER_TICK_UNIT;
    uart_printf(CONSOLE, "Time before %d, Time after %d, Time rec %d\r\n", time_before, time_after, time_rec);

    //uassert(time_after >= (time_before+10));

    




}
