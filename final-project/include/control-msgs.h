#ifndef __CONTROL_MSGS_H__
#define __CONTROL_MSGS_H__

enum MSG_CONTROL_TYPE {
    MSG_CONTROL_QUIT,
    MSG_CONTROL_ACK,
    N_MSG_CONTROL_TYPE
};

typedef struct msg_control {
    enum MSG_CONTROL_TYPE type;
} msg_control;

#endif
