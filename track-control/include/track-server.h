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
    MSG_TS_REQUEST_SEGMENTS_AND_OR_TIMEOUT,
    MSG_TS_REQUEST_SEGMENTS_AND_OR_NO_TIMEOUT,
    MSG_TS_FREE_SEGMENTS,
    MSG_TS_ASK_SEGMENT_FREE,
    MSG_TS_REQUEST_SUCCESS,
    MSG_TS_REQUEST_FAIL,
    MSG_TS_ERROR,
    MSG_TS_NOTIF,
    MSG_TS_MAX
};

enum train_state {
    train_idle = 0,     // no things to do for the server
    train_blocked,  // train in blocked state and waiting for segments to be free
    train_blocked_timeout,   // train in a timeout loop waiting to be done
    train_blocked_two,
    train_blocked_timeout_two
};

typedef struct msg_ts_server {
    enum TS_MSG_TYPE type;
    uint16_t trainNo;
    uint16_t segmentIDs[MAX_SEGMENTS_MSG];
    uint16_t second_segmentIDs[MAX_SEGMENTS_MSG];
    uint8_t no_segments;
    uint8_t second_no_segments;
    uint32_t timeout;
} msg_ts_server;

typedef struct train_locking_structure {
    enum train_state t_state;
    uint8_t trainNo;
    uint16_t requesterTid;

    uint16_t segmentIDs[MAX_SEGMENTS_MSG];
    uint8_t no_segments;

    uint16_t second_segmentIDs[MAX_SEGMENTS_MSG];
    uint8_t second_no_segments;
    bool all_segments_required;

    uint16_t notifierTid;
    uint16_t timeout;
} train_locking_structure;

void track_server_main();
void track_server_timeout_notifier();

#endif
