#ifndef __MSG_TYPE_H__
#define __MSG_TYPE_H__

// define message types for casting
enum MSG_TYPE {
    N_MSG_TYPE
};

// base message type
// receivers can use type field to cast to other types
// and process on those types
typedef struct msg_base {
    enum MSG_TYPE type;
} msg_base;

#endif
