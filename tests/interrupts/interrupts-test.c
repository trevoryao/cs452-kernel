#include "events.h"
#include "sys-clock.h"
#include "rpi.h"
#include "uassert.h"

// run by regular kernel runner, just overwriting user_main used by actual user tasks

#define N_TESTS 1500 // 15 seconds

// just testing that we can indeed run interrupts
void user_main(void) {
    uart_printf(CONSOLE, "starting tick: %u\r\n", get_curr_lo_ticks());

    for (int i = 0; i < N_TESTS; ++i) {
        AwaitEvent(TIMER_TICK);
    }

    uart_printf(CONSOLE, "ending tick: %u\r\n", get_curr_lo_ticks());
}
