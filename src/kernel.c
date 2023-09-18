#include <rpi.h>

int kernel_main(void) {
    uart_printf(1, "hello world\n");

    return 0;
}
