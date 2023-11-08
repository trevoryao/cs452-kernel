#include "marklin-server.h"

#include "task.h"
#include "msg.h"
#include "uart-server.h"
#include "events.h"
#include "deque.h"
#include "nameserver.h"
#include "rpi.h"
#include "uassert.h"

enum MARKLIN_STATE {
    MARKLIN_READY,
    MARKLIN_CMD_SENT,
    MARKLIN_SERVER_BUSY,
    MARKLIN_RECEIVING_SENSOR
};

static const enum MARKLIN_STATE NEXT_MARKLIN_STATE[] = {MARKLIN_CMD_SENT, MARKLIN_SERVER_BUSY, MARKLIN_READY};

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

    Reply(tid, (char *)&msg_response, sizeof(struct msg_uartserver));
}

static void replyWaitingForEmtpy(struct deque *waiting_for_empty) {
    struct msg_uartserver msg_response;
    msg_response.type = MSG_UART_BLOCK_FOR_EMPTY_SENDING;
    msg_response.length = 0;

    while (!deque_empty(waiting_for_empty)) {
        int tid = deque_pop_front(waiting_for_empty);
        msg_response.requesterTid = tid;
        Reply(tid, (char *)&msg_response, sizeof(struct msg_uartserver));
    }
}

static void handleSending(struct deque *fifo_write, struct deque *waiting_for_empty, enum MARKLIN_STATE *state, int *outstanding_bytes) {

    if ((*state == MARKLIN_READY) & !deque_empty(fifo_write)) {
        char c = deque_pop_front(fifo_write);
        uart_putc(MARKLIN, c == ZERO_BYTE ? 0x0 : c);
        *state = MARKLIN_CMD_SENT;

        // set the outstanding_bytes if c is asking for sensor data
        if (c >= S88_BASE && c <= (S88_BASE + N_S88)) {
            *outstanding_bytes = c - S88_BASE;
        } else if (c >= S88_RESET && c <= (S88_RESET + N_S88)) {
            *outstanding_bytes = 2;
        }

        // reply to blocked processes
        if (deque_empty(fifo_write)) {
            replyWaitingForEmtpy(waiting_for_empty);
        }

    }
}

static void advanceState(enum MARKLIN_STATE *state, int cts_state) {
    switch (*state) {
        case MARKLIN_READY: {
            break;
        }

        case MARKLIN_CMD_SENT: {
            if (cts_state == 0) {
                *state = MARKLIN_SERVER_BUSY;
            }
            return;
        }

        case MARKLIN_SERVER_BUSY: {
            if (cts_state == 1) {
                *state = MARKLIN_READY;
            }
            return;
        }

        default: upanic("[Marklin-Server] undefined state %d!\r\n", *state);
    }

}

void marklin_server_main(void) {
    int senderTid;
    int mytid = MyTid();
    struct msg_uartserver msg_startup, msg_received;

    // Structures for storing characters to be read or written
    struct deque fifo_read, fifo_write, waiting_readers, waiting_for_empty;
    deque_init(&fifo_read, 8);
    deque_init(&fifo_write, 8);
    deque_init(&waiting_readers, 5);
    deque_init(&waiting_for_empty, 5);

    // variables needed for block sending bc of sensor data
    int outstanding_bytes = 0;

    // register at nameserver
    RegisterAs(MARKLIN_SERVER_NAME);

    // init CTS notifiers
    msg_startup.requesterTid = mytid;
    msg_startup.type = MSG_UART_NOTIFY_CTS;
    msg_startup.buffer[0] = '\0';

    int cts_notifier = Create(P_NOTIF, marklin_notifier_main);
    Send(cts_notifier, (char *)&msg_startup, sizeof(struct msg_uartserver), (char *)&msg_received, sizeof(struct msg_uartserver));

    int cts_notifier2 = Create(P_NOTIF, marklin_notifier_main);
    Send(cts_notifier2, (char *)&msg_startup, sizeof(struct msg_uartserver), (char *)&msg_received, sizeof(struct msg_uartserver));

    // init Read Notifier
    int read_notifier = Create(P_NOTIF, marklin_notifier_main);
    msg_startup.type = MSG_UART_NOTIFY_READ;
    Send(read_notifier, (char *)&msg_startup, sizeof(struct msg_uartserver), (char *)&msg_received, sizeof(struct msg_uartserver));

    // set variables required for state
    enum MARKLIN_STATE marklin_state = MARKLIN_READY;

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
                // Enqueue the bytes to the buffer
                char *it = msg_received.buffer;
                while (*it) {
                    deque_push_back(&fifo_write, *it);
                    ++it;
                }

                // Reply to the sender
                sendSuccessMsg(senderTid);

                // If ready -> send directly
                if (marklin_state == MARKLIN_READY) {
                    handleSending(&fifo_write, &waiting_for_empty, &marklin_state, &outstanding_bytes);
                }
                break;
            }

            case MSG_UART_NOTIFY_CTS: {
                sendSuccessMsg(senderTid);

                // advance the state
                advanceState(&marklin_state, msg_received.buffer[0]);

                if (marklin_state == MARKLIN_READY) {
                    handleSending(&fifo_write, &waiting_for_empty, &marklin_state, &outstanding_bytes);
                }
                break;
            }

            case MSG_UART_NOTIFY_READ: {
                sendSuccessMsg(senderTid);

                char c = msg_received.buffer[0];
                // reply to all possible waiting processes
                if (deque_empty(&waiting_readers)) {
                    // put it in the queue
                    deque_push_back(&fifo_read, c);
                } else {
                    int responseTid = deque_pop_front(&waiting_readers);
                    replyCharacter(responseTid, c);
                }

                // check if we are receiving sensor data
                if (--outstanding_bytes == 0) {
                    handleSending(&fifo_write, &waiting_for_empty, &marklin_state, &outstanding_bytes);
                }
                break;
            }

            case MSG_UART_BLOCK_FOR_EMPTY_SENDING: {
                if (deque_empty(&fifo_write)) {
                    sendSuccessMsg(senderTid);
                } else {
                    deque_push_back(&waiting_for_empty, senderTid);
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

void marklin_notifier_main(void) {
    int my_tid = MyTid();
    int parent_tid = MyParentTid();

    int senderTid;
    struct msg_uartserver msg, reply, startup;
    msg.requesterTid = my_tid;

    // waiting for startup instruction
    Receive(&senderTid, (char *)&startup, sizeof(struct msg_uartserver));
    // reply to parent
    Reply(senderTid, (char *)&msg, sizeof(struct msg_uartserver));

    for (;;) {
        int response = 0;
        if (startup.type == MSG_UART_NOTIFY_CTS) {
            response = AwaitEvent(MARKLIN_CTS);
            msg.type = MSG_UART_NOTIFY_CTS;
        } else {
            //AwaitEvent(READ);
            response = AwaitEvent(MARKLIN_RX);
            msg.type = MSG_UART_NOTIFY_READ;
        }
        msg.buffer[0] = response;
        msg.buffer[1] = '\0';

        Send(parent_tid, (char*)&msg, sizeof(struct msg_uartserver), (char*)&reply, sizeof(struct msg_uartserver));
    }
}
