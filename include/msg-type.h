#ifndef __MSG_TYPE_H__
#define __MSG_TYPE_H__

#include <stdint.h>

#define MAX_NAME_LENGTH 256

// define message types for casting
enum MSG_TYPE {
    MSG_NAMESERVER_REGISTER,
    MSG_NAMESERVER_WHOIS,
    MSG_NAMESERVER_OK,
    MSG_NAMESERVER_ERROR,
    N_MSG_TYPE
};

// base message type
// receivers can use type field to cast to other types
// and process on those types
typedef struct msg_base {
    enum MSG_TYPE type;
} msg_base;

// Messages for nameserver
typedef struct msg_nameserver {
    enum MSG_TYPE type;
    char name[MAX_NAME_LENGTH];
    uint16_t tid;
} msg_nameserver;

#endif
