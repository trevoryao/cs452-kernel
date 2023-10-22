#include "events.h"
#include "sys-clock.h"
#include "rpi.h"
#include "uassert.h"

// run by regular kernel runner, just overwriting user_main used by actual user tasks

#define N_TESTS 1500 // 15 seconds

// just testing that we can indeed run interrupts
void user_main(void) {
    // CTS interrupts
    for (int i = 0; i < 5; ++i) {
        ULOG("CTS starting: %d\r\n", uart_get_cts_status(MARKLIN));

        uart_putc(MARKLIN, 10);
        // while (uart_get_cts_status(MARKLIN) == 1);
        // ULOG("CTS dropped\r\n");
        // while (uart_get_cts_status(MARKLIN) == 0);
        // ULOG("CTS rose\r\n");

        AwaitEvent(MARKLIN_CTS); // lo
        AwaitEvent(MARKLIN_CTS); // hi

        ULOG("first byte send\r\n");

        uart_putc(MARKLIN, 24);
        // while (uart_get_cts_status(MARKLIN) == 1);
        // ULOG("CTS dropped\r\n");
        // while (uart_get_cts_status(MARKLIN) == 0);
        // ULOG("CTS rose\r\n");


        AwaitEvent(MARKLIN_CTS); // lo
        AwaitEvent(MARKLIN_CTS); // hi

        ULOG("seccond byte send\r\n");
    }

    // timer interrupts
    // uart_printf(CONSOLE, "starting tick: %u\r\n", get_curr_lo_ticks());

    // for (int i = 0; i < N_TESTS; ++i) {
    //     AwaitEvent(TIMER_TICK);
    // }

    // uart_printf(CONSOLE, "ending tick: %u\r\n", get_curr_lo_ticks());
}
