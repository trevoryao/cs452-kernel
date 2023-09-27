#ifndef _rpi_h_
#define _rpi_h_ 1

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

// here line denotes the serial channel
enum LINE {
    CONSOLE = 1,
    LINE
};

#define NCH -1

unsigned char uart_getc(size_t line);
short uart_getc_nb(size_t line); // non blocking getc, return NCH if no char in UART buffer

void uart_putc(size_t line, unsigned char c);
void uart_putl(size_t line, const char *buf, size_t blen);
void uart_puts(size_t line, const char *buf);
void uart_printf(size_t line, char *fmt, ...);

#ifdef LOG
#define KLOG(fmt, ...) uart_printf(CONSOLE, "[KLOG] " fmt __VA_OPT__(,) __VA_ARGS__)
#else
#define KLOG(...)
#endif

bool uart_out_empty(size_t line);

void uart_config_and_enable(size_t line, uint32_t baudrate, uint32_t stopbit);
void uart_init();

#endif /* rpi.h */
