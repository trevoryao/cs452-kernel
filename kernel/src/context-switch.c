#include "context-switch.h"

// lib
#include "rpi.h"

extern int get_el(void);

void unimplemented_handler(void *addr, uint64_t esr) {
    uart_printf(CONSOLE, "Received Exception/Interrupt at %x [%x]\r\n", addr, esr);
}

void print_el() {
    uart_printf(CONSOLE, "Current Exception Level: %d\r\n", get_el());
}
