#include "console-server.h"

#include "nameserver.h"
#include "events.h"
#include "msg.h"
#include "task.h"
#include "uart-server.h"
#include "rpi.h"
#include "uassert.h"
#include "deque.h"
#include "clock-server.h"
#include "clock.h"

#define READ_DELAY_TICKS 3

static void sendErrorMsg(int tid) {
    struct msg_uartserver msg_error;
    msg_error.type = MSG_UART_ERROR;
    msg_error.requesterTid = tid;
    msg_error.buffer[0] = '\0';

    Reply(tid, (char *)&msg_error, sizeof(struct msg_uartserver));
}

static void sendSuccessMsg(int tid) {
    struct msg_uartserver msg_success;
    msg_success.type = MSG_UART_SUCCESS;
    msg_success.requesterTid = tid;
    msg_success.buffer[0] = '\0';

    Reply(tid, (char *)&msg_success, sizeof(struct msg_uartserver));
}

static void replyCharacter(int tid, char c) {
    struct msg_uartserver msg_response;
    msg_response.type = MSG_UART_GETC;
    msg_response.requesterTid = tid;
    msg_response.buffer[0] = c;
    msg_response.buffer[1] = '\0';

    Reply(tid, (char *)&msg_response, sizeof(struct msg_uartserver));
}

void console_server_main(void) {
    int senderTid;
    struct msg_uartserver msg_received;

    struct deque fifo_read, waiting_readers;
    deque_init(&fifo_read, 4);
    deque_init(&waiting_readers, 3);

    RegisterAs(CONSOLE_SERVER_NAME);

    // startup read notifier
    Create(P_NOTIF, console_read_notifer_main);


    for (;;) {
        Receive(&senderTid, (char *)&msg_received, sizeof(struct msg_uartserver));

        switch (msg_received.type) {
            case MSG_UART_GETC: {
                // check if sth can be returned
                if (deque_empty(&fifo_read)) {
                    // add process to the waiting queue
                    deque_push_back(&waiting_readers, msg_received.requesterTid);
                } else {
                    char response = (char) deque_pop_front(&fifo_read);
                    replyCharacter(msg_received.requesterTid, response);
                }
                break;
            }

            case MSG_UART_PUTS: {
                sendSuccessMsg(senderTid);
                uart_puts(CONSOLE, msg_received.buffer);
                break;
            }

            case MSG_UART_NOTIFY_READ: {
                sendSuccessMsg(senderTid);

                for(int i = 0; i < msg_received.length; i++){
                    char c = msg_received.buffer[i];
                    deque_push_back(&fifo_read, c);
                }

                // reply to all possible waiting processes
                while (!deque_empty(&waiting_readers) && !deque_empty(&fifo_read)) {
                    // put it in the queue
                    int responseTid = deque_pop_front(&waiting_readers);
                    char c = deque_pop_front(&fifo_read);
                    replyCharacter(responseTid, c);
                }
                break;
            }

            default: {
                sendErrorMsg(senderTid);
                break;
            }
        }
    }
}

void console_read_notifer_main(void) {
    int my_tid = MyTid();
    int parent_tid = MyParentTid();

    struct msg_uartserver msg, reply;
    msg.requesterTid = my_tid;
    msg.type = MSG_UART_NOTIFY_READ;

    msg.length = 0;

    for(;;) {
        int c = uart_getc_nb(CONSOLE);

        switch (c) {
            case NCH: {
                if (msg.length > 0) {
                    Send(parent_tid, (char*)&msg, sizeof(struct msg_uartserver), (char*)&reply, sizeof(struct msg_uartserver));
                    msg.length = 0;
                }

                int result = AwaitEvent(CONSOLE_RX);
                if (result < 0) {
                    upanic("[Console Read Notifier] Console Server error - make sure to start a clock server\r\n");
                }

                break;
            }

            // check arrow keys
            case 'A':
            case 'B':
            case 'C':
            case 'D': {
                if (msg.length >= 2 && msg.buffer[msg.length - 2] == 033 &&
                    msg.buffer[msg.length - 1] == '[') {
                    // arrow key?
                    msg.length -= 2;
                    msg.buffer[msg.length++] = ARROW_UP + c - 'A';
                    break;
                } __attribute__((fallthrough));
            }
            default: {
                msg.buffer[msg.length++] = c;
                break;
            }
        }
    }
}
