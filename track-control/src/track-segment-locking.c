#include "track-segment-locking.h"

#include <stdint.h>
#include "util.h"
#include "msg.h"
#include "track-server.h"

bool track_server_available(int tid) {
    struct msg_ts_server msg_request, msg_reply;
    msg_request.requesterTid = tid;
    msg_request.node = NULL;
    msg_request.type = MSG_TS_SERVER_AVAILABLE;

    Send(tid, (char *)&msg_request, sizeof(struct msg_ts_server), (char *)&msg_reply, sizeof(struct msg_ts_server));
    return (msg_reply.type == MSG_TS_REQUEST_SUCCESS);
}

bool track_server_acquire_server_lock(int tid, uint16_t trainNo) {
    struct msg_ts_server msg_request, msg_reply;
    msg_request.requesterTid = tid;
    msg_request.node = NULL;
    msg_request.type = MSG_TS_ACQUIRE_SERVER_LOCK;
    msg_request.trainNo = trainNo;

    Send(tid, (char *)&msg_request, sizeof(struct msg_ts_server), (char *)&msg_reply, sizeof(struct msg_ts_server));
    return (msg_reply.type == MSG_TS_REQUEST_SUCCESS);
}

void track_server_free_server_lock(int tid, uint16_t trainNo) {
    struct msg_ts_server msg_request, msg_reply;
    msg_request.requesterTid = tid;
    msg_request.node = NULL;
    msg_request.type = MSG_TS_FREE_SERVER_LOCK;
    msg_request.trainNo = trainNo;

    Send(tid, (char *)&msg_request, sizeof(struct msg_ts_server), (char *)&msg_reply, sizeof(struct msg_ts_server));
}

bool track_server_lock_segment(int tid, track_node *node, uint16_t trainNo) {
    struct msg_ts_server msg_request, msg_reply;
    msg_request.requesterTid = tid;
    msg_request.node = node;
    msg_request.type = MSG_TS_REQUEST_SEGMENT;
    msg_request.trainNo = trainNo;

    Send(tid, (char *)&msg_request, sizeof(struct msg_ts_server), (char *)&msg_reply, sizeof(struct msg_ts_server));
    return (msg_reply.type == MSG_TS_REQUEST_SUCCESS);
}

void track_server_free_segment(int tid, track_node *node, uint16_t trainNo) {
    struct msg_ts_server msg_request, msg_reply;
    msg_request.requesterTid = tid;
    msg_request.node = node;
    msg_request.type = MSG_TS_FREE_SEGMENT;
    msg_request.trainNo = trainNo;

    Send(tid, (char *)&msg_request, sizeof(struct msg_ts_server), (char *)&msg_reply, sizeof(struct msg_ts_server));
}

bool track_server_is_locked(int tid, track_node *node, uint16_t trainNo) {
    struct msg_ts_server msg_request, msg_reply;
    msg_request.requesterTid = tid;
    msg_request.node = node;
    msg_request.type = MSG_TS_CHECK_SEGMENT;
    msg_request.trainNo = trainNo;

    Send(tid, (char *)&msg_request, sizeof(struct msg_ts_server), (char *)&msg_reply, sizeof(struct msg_ts_server));
    return (msg_reply.type == MSG_TS_REQUEST_SUCCESS);
}
