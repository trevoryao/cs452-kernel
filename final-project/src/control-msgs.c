#include "control-msgs.h"

#include "msg.h"
#include "uassert.h"

void BlockUntilReady(void) {
    msg_control msg;
    int ctid;
    uassert(Receive(&ctid, (char *)&msg, sizeof(msg_control)) == sizeof(msg_control));
    uassert(msg.type == MSG_CONTROL_READY); // unrecoverable if we received anything before being ready
    msg.type = MSG_CONTROL_ACK;
    uassert(Reply(ctid, (char *)&msg, sizeof(msg_control)) == sizeof(msg_control));
}

void SendParentReady() {
    msg_control msg = { MSG_CONTROL_READY };
    int ptid = MyParentTid();
    uassert(Send(ptid, (char *)&msg, sizeof(msg_control),
        (char *)&msg, sizeof(msg_control)) == sizeof(msg_control));
    uassert(msg.type == MSG_CONTROL_ACK);
}

void BlockUntilQuit() {
    msg_control msg;
    int ctid;
    uassert(Receive(&ctid, (char *)&msg, sizeof(msg_control)) == sizeof(msg_control));
    uassert(msg.type == MSG_CONTROL_QUIT); // unrecoverable if we received anything before being ready
    msg.type = MSG_CONTROL_ACK;
    uassert(Reply(ctid, (char *)&msg, sizeof(msg_control)) == sizeof(msg_control));
}

void SendParentQuit() {
    msg_control msg = { MSG_CONTROL_QUIT };
    int ptid = MyParentTid();
    uassert(Send(ptid, (char *)&msg, sizeof(msg_control),
        (char *)&msg, sizeof(msg_control)) == sizeof(msg_control));
    uassert(msg.type == MSG_CONTROL_ACK);
}

