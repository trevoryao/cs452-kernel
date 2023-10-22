#include "uart-server.h"

#include <stdarg.h>

#include "msg.h"
#include "task.h"
#include "util.h"

int Putc(int tid, unsigned char ch) {
    // set header
    struct msg_uartserver msg;
    msg.requesterTid = MyTid();
    msg.type = MSG_UART_PUTS;

    // set message
    msg.buffer[0] = ch;
    msg.buffer[1] = '\0';

    int ret = Send(tid, (char *)&msg, sizeof(struct msg_uartserver), (char *)&msg, sizeof(struct msg_uartserver));

    return (ret < 0 || msg.type == MSG_UART_ERROR) ? -1 : 0;
}

int Getc(int tid) {
    // set header
    struct msg_uartserver msg;
    msg.requesterTid = MyTid();
    msg.type = MSG_UART_GETC;

    int ret = Send(tid, (char *)&msg, sizeof(struct msg_uartserver), (char *)&msg, sizeof(struct msg_uartserver));

    return (ret < 0 || msg.type == MSG_UART_ERROR) ? -1 : msg.buffer[0];
}

int Putl(int tid, const char* buf, size_t blen) {
    // prevent overflow -> return error
    if (blen > MAX_STRING_LENGTH) {
        return -1;
    }

    // set header
    struct msg_uartserver msg;
    msg.requesterTid = MyTid();
    msg.type = MSG_UART_PUTS;

    uint32_t i;
    for(i = 0; i < blen; i++) {
        msg.buffer[i] = *(buf+i);
    }

    msg.buffer[i] = '\0';

    int ret = Send(tid, (char *)&msg, sizeof(struct msg_uartserver), (char *)&msg, sizeof(struct msg_uartserver));
    return (ret < 0 || msg.type == MSG_UART_ERROR) ? -1 : 0;
}

int Puts(int tid, const char* buf) {
    // set header
    struct msg_uartserver msg;
    msg.requesterTid = MyTid();
    msg.type = MSG_UART_PUTS;

    uint32_t i = 0;
    while (*buf && i < MAX_STRING_LENGTH) {
	    msg.buffer[i] = *buf;
	    buf++;
        i++;
    }
    msg.buffer[i] = '\0';

    int ret = Send(tid, (char *)&msg, sizeof(struct msg_uartserver), (char *)&msg, sizeof(struct msg_uartserver));
    return (ret < 0 || msg.type == MSG_UART_ERROR) ? -1 : 0;
}

static void uart_server_buffer_putc(struct msg_uartserver *msg, int *length, char ch) {
    msg->buffer[*length] = ch;
    *length += 1;
}

static void uart_server_buffer_puts(struct msg_uartserver *msg, int *length, const char* buf) {
    while (*buf && (*length < MAX_STRING_LENGTH - 1)) {
        msg->buffer[*length] = *buf;
        buf++;
        *length += 1;
    }
}

// printf-style printing, with limited format support
static void uart_server_format_print(struct msg_uartserver *msg, char *fmt, va_list va) {
    char bf[12];
    char ch;

    int length = 0;

    while ( ( ch = *(fmt++) ) && (length < MAX_STRING_LENGTH - 1)) {
	    if ( ch != '%' ) {
            uart_server_buffer_putc(msg, &length, ch);
        } else {
			ch = *(fmt++);
			switch( ch ) {
			    case 'u':
				    ui2a( va_arg( va, unsigned int ), 10, bf );
				    uart_server_buffer_puts(msg, &length, bf);
				    break;
			    case 'd':
				    i2a( va_arg( va, int ), bf );
				    uart_server_buffer_puts(msg, &length, bf);
				    break;
			    case 'x':
                    uart_server_buffer_puts(msg, &length, "0x");
				    ui2a( va_arg( va, unsigned int ), 16, bf );
				    uart_server_buffer_puts(msg, &length, bf);
				    break;
			    case 's':
				    uart_server_buffer_puts(msg, &length, va_arg( va, char* ) );
		            break;
                case 'c':
                    uart_server_buffer_putc(msg, &length, va_arg(va, int));
		            break;
                case '%':
				    uart_server_buffer_putc(msg, &length, ch );
				    break;
                case '\0':
		            return;
			}
        }
    }

    msg->buffer[length] = '\0';
}

int Printf(int tid, char *fmt, ... ) {
	va_list va;

    struct msg_uartserver msg;
    msg.type = MSG_UART_PUTS;
    msg.requesterTid = MyTid();

	va_start(va,fmt);
	uart_server_format_print(&msg, fmt, va );
    va_end(va);

    int ret = Send(tid, (char *)&msg, sizeof(struct msg_uartserver), (char *)&msg, sizeof(struct msg_uartserver));
    return (ret < 0 || msg.type == MSG_UART_ERROR) ? -1 : 0;
}

int WaitOutputEmpty(int tid) {
    // set header
    struct msg_uartserver msg;
    msg.requesterTid = MyTid();
    msg.type = MSG_UART_BLOCK_FOR_EMPTY_SENDING;

    int ret = Send(tid, (char *)&msg, sizeof(struct msg_uartserver), (char *)&msg, sizeof(struct msg_uartserver));

    return (ret < 0 || msg.type == MSG_UART_ERROR) ? -1 : 0;
}



