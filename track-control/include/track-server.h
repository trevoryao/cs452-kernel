#ifndef __TRACK_SERVER_H__
#define __TRACK_SERVER_H__

#include <stdbool.h>
#include <stdint.h>
#include "track-data.h"
#include "track-node.h"

enum TS_MSG_TYPE {
    MSG_TS_REQUEST_SEGMENT,
    MSG_TS_FREE_SEGMENT,
    MSG_TS_CHECK_SEGMENT,
    MSG_TS_SERVER_AVAILABLE,
    MSG_TS_ACQUIRE_SERVER_LOCK,
    MSG_TS_FREE_SERVER_LOCK,
    MSG_TS_REQUEST_SUCCESS,
    MSG_TS_REQUEST_FAIL,
    MSG_TS_ERROR,
    MSG_TS_MAX
};

typedef struct msg_ts_server {
    enum TS_MSG_TYPE type;
    uint16_t trainNo;
    uint16_t requesterTid;
    track_node *node;
} msg_ts_server;


void track_server_main();

#endif
