#include "rpi.h"
#include "clock.h"


void user_main(void) {
    uart_printf(CONSOLE, "this is test");
}