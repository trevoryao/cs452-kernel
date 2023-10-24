#ifndef __UART_SERVER_H__
#define __UART_SERVER_H__

#include <stdint.h>
#include "msg-type.h"
#include <stddef.h> // TODO: needed for size_t

#define MAX_STRING_LENGTH 256
#define CONSOLE_SERVER_NAME "console-server"
#define MARKLIN_SERVER_NAME "marklin-server"

#define ZERO_BYTE 0xFF

// sensor codes
#define S88_BASE 128
#define N_S88 5

#define S88_RESET 192

enum UART_MSG_TYPE {
    MSG_UART_PUTS,
    MSG_UART_GETC,
    MSG_UART_NOTIFY_READ,
    MSG_UART_NOTIFY_CTS,
    MSG_UART_ERROR,
    MSG_UART_SUCCESS,
    MSG_UART_BLOCK_FOR_EMPTY_SENDING,
    MSG_UART_MAX
};

typedef struct msg_uartserver {
    enum UART_MSG_TYPE type;
    uint16_t requesterTid;
    char buffer[MAX_STRING_LENGTH];
    int length;
} msg_uartserver;

/*
* returns the next un-returned character from the given channel.
* The first argument is the task id of the appropriate I/O server.
* How communication errors are handled is implementation-dependent.
* Getc() is actually a wrapper for a send to the appropriate server.
*
* Return Value
* >=0	new character from the given UART.
* -1	tid is not a valid uart server task.
*/
int Getc(int tid);

/*
* queues the given character for transmission by the given UART.
* On return the only guarantee is that the character has been queued.
* Whether it has been transmitted or received is not guaranteed.
* How communication errors are handled is implementation-dependent.
* Putc() is actually a wrapper for a send to the appropriate server.
*
* Return Value
* 0	success.
* -1	tid is not a valid uart server task.
*/
int Putc(int tid, unsigned char ch);

/*
* Usual printing functions, on top of Getc.
*
* Return Value
* 0	success.
* -1	tid is not a valid uart server task.
*/

int Putl(int tid, const char *buf, size_t blen);
int Puts(int tid, const char *buf);
int Printf(int tid, char *fmt, ...);

/*
* Blocks the calling task until the transmission buffer for the given UART is empty
* If the uart is already empty, will return without blocking.
* Return Value
* 0	success.
* -1	tid is not a valid uart server task.
*/
int WaitOutputEmpty(int tid);

#endif
