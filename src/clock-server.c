#include "clock-server.h"

#include "clock-queue.h"
#include "clock.h"
#include "task.h"
#include "msg.h"
#include "events.h"
#include "nameserver.h"
#include "uassert.h"
#include "sys-clock.h"

static void sendErrorMsg(int tid) {
    struct msg_clockserver msg_error; // TODO
    msg_error.type = MSG_CLOCKSERVER_ERROR;
    msg_error.requesterTid = tid;
    msg_error.clockTick = 0;

    Reply(tid, (char *)&msg_error, sizeof(struct msg_clockserver));
}

static void replyCurrentClockTick(int tid) {
    struct msg_clockserver msg_reply;
    msg_reply.type = MSG_CLOCKSERVER_TIME;
    msg_reply.requesterTid = tid;
    msg_reply.clockTick = get_curr_ticks();

    Reply(tid, (char *)&msg_reply, sizeof(struct msg_clockserver));
}

static void dequeueAllReady(clock_queue *waitingProcesses) {
    uint64_t currentClockTick = get_curr_ticks();

    while (clock_queue_ready(waitingProcesses, currentClockTick)) {
        uint16_t readyTid = clock_queue_get(waitingProcesses, currentClockTick);
        ULOG("dequeing %u\r\n", readyTid);

        if (readyTid == 0) {
            // TODO do error handling
            sendErrorMsg(readyTid);
            break;
        } else {
            struct msg_clockserver msg_reply;
            msg_reply.clockTick = currentClockTick;
            msg_reply.requesterTid = readyTid;
            msg_reply.type = MSG_CLOCKSERVER_AWAKE;

            Reply(readyTid, (char *)&msg_reply, sizeof(struct msg_clockserver));
        }
    }
}

static void replyNotifier(int notifierTid) {
    struct msg_clockserver msg_reply;
    msg_reply.clockTick = get_curr_ticks();
    msg_reply.requesterTid = notifierTid;
    msg_reply.type = MSG_CLOCKSERVER_OK;

    Reply(notifierTid, (char *)&msg_reply, sizeof(struct msg_clockserver));
}

void clockserver_main() {
    int senderTid;
    struct msg_clockserver msg_received;
    clock_queue waitingProcesses;

    // register at nameserver
    RegisterAs(CLOCK_SERVER_NAME);

    // init queue
    clock_queue_init(&waitingProcesses);

    // init notifier
    Create(P_NOTIF, clocknotifier_main);

    ULOG("[Clockserver] init done\r\n");

    // infinite main loop
    for(;;) {
        Receive(&senderTid, (char *)&msg_received, sizeof(struct msg_clockserver));

        ULOG("[Clockserver] rcv'd msg %d\r\n", msg_received.type);

        switch (msg_received.type) {
            case MSG_CLOCKSERVER_TIME: {
                // reply the time directly (from saved variable)
                replyCurrentClockTick(msg_received.requesterTid);
                break;
            }
            case MSG_CLOCKSERVER_DELAY: {
                // delay time with time amount
                uint64_t currentClockTick = get_curr_ticks();
                uint64_t delayTime = currentClockTick + msg_received.clockTick;
                clock_queue_add(&waitingProcesses, msg_received.requesterTid, delayTime); // delay until
                break;
            }
            case MSG_CLOCKSERVER_DELAY_UNTIL: {
                // same as delay but without addition
                clock_queue_add(&waitingProcesses, msg_received.requesterTid, msg_received.clockTick); // delay until
                break;
            }
            case MSG_CLOCKSERVER_NOTIFY: {
                // send out replies
                dequeueAllReady(&waitingProcesses);

                // reply the notify server
                replyNotifier(senderTid);
                break;
            }
            default: {
                sendErrorMsg(senderTid);
                break;
            }
        }
    }
}

void clocknotifier_main() {
    int my_tid = MyTid();
    int parent_tid = MyParentTid();

    struct msg_clockserver msg;
    msg.requesterTid = my_tid;
    msg.type = MSG_CLOCKSERVER_NOTIFY;

    struct msg_clockserver reply;

    for (;;) {
        // Use Await System Call to wait for the next clocktick
        AwaitEvent(TIMER_TICK);

        Send(parent_tid, (char*)&msg, sizeof(struct msg_clockserver), (char*)&reply, sizeof(struct msg_clockserver));
    }
}