#include "clock.h"

#include "msg.h"
#include "task.h"
#include "clock-server.h"
#include "sys-clock.h"
#include "rpi.h"

/* Send/rcv wrappers */

int Time(int tid) {
    uint16_t my_tid = MyTid();

    struct msg_clockserver msg;
    msg.type = MSG_CLOCKSERVER_TIME;
    msg.requesterTid = my_tid;

    int ret = Send(tid, (char *)&msg, sizeof(struct msg_clockserver), (char *)&msg, sizeof(struct msg_clockserver));
    
    if (ret < 0 || msg.type == MSG_CLOCKSERVER_ERROR) {
        return -1;
    } else {
        return msg.clockTick / TIMER_TICK_UNIT;
    }
}

int Delay(int tid, int ticks) {
    uint16_t my_tid = MyTid();

    if (ticks < 0) {
        return -2;
    }

    struct msg_clockserver msg;
    msg.type = MSG_CLOCKSERVER_DELAY;
    msg.requesterTid = my_tid;
    msg.clockTick = (uint32_t)(TIMER_TICK_UNIT * ticks);

    int ret = Send(tid, (char *)&msg, sizeof(struct msg_clockserver), (char *)&msg, sizeof(struct msg_clockserver));

    if (ret < 0 || msg.type == MSG_CLOCKSERVER_ERROR) {
        return -1;
    } else {
        return msg.clockTick / TIMER_TICK_UNIT;
    }
}


int DelayUntil(int tid, int ticks) {
    uint16_t my_tid = MyTid();
    uint64_t realTicks = (uint64_t)(TIMER_TICK_UNIT * ticks);
    if (realTicks < get_curr_ticks()) {
        return -2;
    }

    struct msg_clockserver msg;
    msg.type = MSG_CLOCKSERVER_DELAY_UNTIL;
    msg.requesterTid = my_tid;
    msg.clockTick = realTicks;

    int ret = Send(tid, (char *)&msg, sizeof(struct msg_clockserver), (char *)&msg, sizeof(struct msg_clockserver));

    if (ret < 0 || msg.type == MSG_CLOCKSERVER_ERROR) {
        return -1;
    } else {
        return msg.clockTick / TIMER_TICK_UNIT;
    }
}
