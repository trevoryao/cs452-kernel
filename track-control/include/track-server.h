#ifndef __TRACK_SERVER_H__
#define __TRACK_SERVER_H__

#include <stdbool.h>
#include <stdint.h>
#include "track-data.h"
#include "track-node.h"

#define N_SEGMENTS 31
#define MAX_SEGMENTS_MSG 8

enum TS_MSG_TYPE {
    MSG_TS_REQUEST_SEGMENTS_ALL_TIMEOUT,
    MSG_TS_REQUEST_SEGMENTS_ONE_TIMEOUT,
    MSG_TS_REQUEST_SEGMENTS_ALL_NO_TIMEOUT,
    MSG_TS_REQUEST_SEGMENTS_ONE_NO_TIMEOUT,
    MSG_TS_FREE_SEGMENTS,
    MSG_TS_ASK_SEGMENT_FREE,
    MSG_TS_REQUEST_SUCCESS,
    MSG_TS_REQUEST_FAIL,
    MSG_TS_ERROR,
    MSG_TS_MAX
};

typedef struct msg_ts_server {
    enum TS_MSG_TYPE type;
    uint16_t trainNo;
    uint16_t segmentIDs[MAX_SEGMENTS_MSG];
    uint8_t no_segments;
    uint32_t timeout;
} msg_ts_server;


void track_server_main();

#endif
