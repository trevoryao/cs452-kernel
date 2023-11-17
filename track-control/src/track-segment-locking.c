#include "track-segment-locking.h"

#include <stdint.h>
#include "util.h"
#include "msg.h"
#include "track-server.h"
#include "deque.h"

// TODO -> sanity check if segment is in allowed range!!!!
void copySegmentIDs(msg_ts_server *msg, deque *segments) {
    msg->no_segments = 0;

    while (!deque_empty(segments)) {
        msg->segmentIDs[msg->no_segments] = deque_pop_front(segments);
        msg->no_segments += 1;
    }
}

bool track_server_lock_all_segments_timeout(int tid, deque *segmentIDs, uint16_t trainNo, uint32_t timeout_ticks) {
    struct msg_ts_server msg_request, msg_reply;
    msg_request.type = MSG_TS_REQUEST_SEGMENTS_ALL_TIMEOUT;
    msg_request.timeout = timeout_ticks;
    msg_request.trainNo = trainNo;

    // copy over the segments
    copySegmentIDs(&msg_request, segmentIDs);

    Send(tid, (char *)&msg_request, sizeof(struct msg_ts_server), (char *)&msg_reply, sizeof(struct msg_ts_server));

    return (msg_reply.type == MSG_TS_REQUEST_SUCCESS);
}


void track_server_lock_all_segments(int tid, deque *segmentIDs, uint16_t trainNo) {
    struct msg_ts_server msg_request, msg_reply;
    msg_request.type = MSG_TS_REQUEST_SEGMENTS_ALL_NO_TIMEOUT;
    msg_request.timeout = 0;
    msg_request.trainNo = trainNo;

    copySegmentIDs(&msg_request, segmentIDs);

    Send(tid, (char *)&msg_request, sizeof(struct msg_ts_server), (char *)&msg_reply, sizeof(struct msg_ts_server));
}

int track_server_lock_one_segment_timeout(int tid, deque *segmentIDs, uint16_t trainNo, uint32_t timeout_ticks) {
    struct msg_ts_server msg_request, msg_reply;
    msg_request.type = MSG_TS_REQUEST_SEGMENTS_ONE_TIMEOUT;
    msg_request.timeout = timeout_ticks;
    msg_request.trainNo = trainNo;

    copySegmentIDs(&msg_request, segmentIDs);

   
    Send(tid, (char *)&msg_request, sizeof(struct msg_ts_server), (char *)&msg_reply, sizeof(struct msg_ts_server));
    if (msg_reply.type == MSG_TS_REQUEST_SUCCESS) {
        return msg_reply.segmentIDs[0];
    } else {
        return -1;
    }
}

int track_server_lock_one_segment(int tid, deque *segmentIDs, uint16_t trainNo) {
    struct msg_ts_server msg_request, msg_reply;
    msg_request.type = MSG_TS_REQUEST_SEGMENTS_ONE_NO_TIMEOUT;
    msg_request.timeout = 0;
    msg_request.trainNo = trainNo;

    copySegmentIDs(&msg_request, segmentIDs);

    Send(tid, (char *)&msg_request, sizeof(struct msg_ts_server), (char *)&msg_reply, sizeof(struct msg_ts_server));
    if (msg_reply.type == MSG_TS_REQUEST_SUCCESS) {
        return msg_reply.segmentIDs[0];
    } else {
        return -1;
    }
}

void track_server_free_segments(int tid, deque *segmentIDs, uint16_t trainNo) {
    struct msg_ts_server msg_request, msg_reply;
    msg_request.type = MSG_TS_FREE_SEGMENTS;
    msg_request.timeout = 0;
    msg_request.trainNo = trainNo;

    copySegmentIDs(&msg_request, segmentIDs);

    Send(tid, (char *)&msg_request, sizeof(struct msg_ts_server), (char *)&msg_reply, sizeof(struct msg_ts_server));
}

bool track_server_segment_is_locked(int tid, int segmentID, uint16_t trainNo) {
    struct msg_ts_server msg_request, msg_reply;
    msg_request.type = MSG_TS_ASK_SEGMENT_FREE;
    msg_request.timeout = 0;
    msg_request.trainNo = trainNo;
    msg_request.no_segments = 1;
    msg_request.segmentIDs[0] = segmentID;


    Send(tid, (char *)&msg_request, sizeof(struct msg_ts_server), (char *)&msg_reply, sizeof(struct msg_ts_server));

    return (msg_reply.type == MSG_TS_REQUEST_SUCCESS);
}
