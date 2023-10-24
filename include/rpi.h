#ifndef _rpi_h_
#define _rpi_h_ 1

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

// here line denotes the serial channel
enum LINE {
    CONSOLE = 1,
    MARKLIN
};

#define NCH -1

unsigned char uart_getc(size_t line);
short uart_getc_nb(size_t line); // non blocking getc, return NCH if no char in UART buffer

void uart_putc(size_t line, unsigned char c);
void uart_putl(size_t line, const char *buf, size_t blen);
void uart_puts(size_t line, const char *buf);
void uart_printf(size_t line, char *fmt, ...);

// masks for specific fields in interrupt return value
static const uint32_t UART_I_CTS =  0x02;
static const uint32_t UART_I_RX =   0x10;
static const uint32_t UART_I_RTIM = 0x40;

uint32_t uart_get_interrupts(size_t line);
void uart_clear_interrupt(size_t line, uint32_t msk);

// returns actual CTS status (0 for lo, 1 for hi)
int uart_get_cts_status(size_t line);
bool uart_is_tx_clear(size_t line);
bool uart_is_rx_clear(size_t line);

void uart_config_and_enable_console();
void uart_config_and_enable_marklin();

void uart_init();

#endif /* rpi.h */
