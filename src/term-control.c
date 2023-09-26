#include "term-control.h"

// lib
#include "rpi.h"

void curs_move(uint16_t r, uint16_t c) {
    uart_printf(CONSOLE, "\033[%u;%uH", r, c);
}
