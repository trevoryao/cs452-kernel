#include "track-server.h"

#include "deque.h"
#include "msg.h"
#include "task.h"
#include "nameserver.h"
#include "clock.h"
#include "track.h"
#include "track-data.h"
#include "track-node.h"
#include "track-segment-locking.h"
#include "util.h"
#include "rpi.h"
#include "speed-data.h"
#include "clock.h"
#include "task.h"
#include "uassert.h"
#include "speed.h"
#include "monitor.h"

extern track_node track[];

static void reply(int tid, bool success) {
    struct msg_ts_server msg_reply;
    msg_reply.type = success ? MSG_TS_REQUEST_SUCCESS : MSG_TS_REQUEST_FAIL;

    Reply(tid, (char *)&msg_reply, sizeof(struct msg_ts_server));
}

static void replySegment(int tid, int segmentID) {
    struct msg_ts_server msg_reply;
    msg_reply.type = (segmentID == -1) ? MSG_TS_REQUEST_FAIL : MSG_TS_REQUEST_SUCCESS;
    msg_reply.segmentIDs[0] = segmentID;
    msg_reply.no_segments = 1;

    Reply(tid, (char *)&msg_reply, sizeof(struct msg_ts_server));
}

static void replyError(int tid) {
    struct msg_ts_server msg_reply;
    msg_reply.type = MSG_TS_ERROR;

    Reply(tid, (char *)&msg_reply, sizeof(struct msg_ts_server));
}

static bool checkAllFree(uint16_t *lock_segments, uint16_t *requested_segments, uint16_t no_requested_segments, uint16_t trainNo) {
    for (int i = 0; i < no_requested_segments; i++) {
        int segment = requested_segments[i];
        if (lock_segments[segment] != 0 && lock_segments[segment] != trainNo) {
            return false;
        }
    }
    return true;
}

static void lockAll_Unsafe(int console, uint16_t *lock_segments, uint16_t *requested_segments, uint16_t no_requested_segments, uint16_t trainNo,  uint8_t *next_segment, train_locking_structure *train_data) {
    for (int i = 0; i < no_requested_segments; i++) {
        int segment = requested_segments[i];
        lock_segments[segment] = trainNo;
        update_segment(console, segment, trainNo);

        next_segment[segment] = train_data->curr_index;
        train_data->curr_index += 1;
    }
}

static int lockOne_Safe(int console, uint16_t *lock_segments, uint16_t *requested_segments, uint16_t no_requested_segments, uint16_t trainNo, uint8_t *next_segment, train_locking_structure *train_data) {
    // go through all segments and return the first free segment
    for (int i = 0; i < no_requested_segments; i++) {
        int requested_id = requested_segments[i];
        if (lock_segments[requested_id] == 0 || lock_segments[requested_id] == trainNo) {
            lock_segments[requested_id] = trainNo;
            update_segment(console, requested_id, trainNo);

            // update the data for next segement
            next_segment[requested_id] = train_data->curr_index;
            train_data->curr_index += 1;
            return requested_id;
        }
    }

    // return -1 if no segment could be locked
    return -1;
}

static bool lockAll_Safe(int console, uint16_t *lock_segments, uint16_t *requested_segments, uint16_t no_requested_segments, uint16_t trainNo,  uint8_t *next_segment, train_locking_structure *train_data) {
    // check if all are free
    if (checkAllFree(lock_segments, requested_segments, no_requested_segments, trainNo)) {
        lockAll_Unsafe(console, lock_segments, requested_segments, no_requested_segments, trainNo, next_segment, train_data);
        return true;
    } else {
        return false;
    }
}

static void replyNotifier(int tid, uint16_t trainNo, uint16_t timeout) {
    msg_ts_server msg;
    msg.type = MSG_TS_NOTIF;
    msg.timeout = timeout;
    msg.trainNo = trainNo;

    Reply(tid, (char *)&msg, sizeof(msg_ts_server));
}

static void copyTrainData(train_locking_structure *train_data, msg_ts_server *msg) {
    train_data->no_segments = msg->no_segments;
    train_data->second_no_segments = msg->second_no_segments;
    train_data->trainNo = msg->trainNo;
    train_data->timeout = msg->timeout;

    memcpy(train_data->segmentIDs, msg->segmentIDs, sizeof(uint16_t) * MAX_SEGMENTS_MSG);

    // copy the second segmentIDs
    memcpy(train_data->second_segmentIDs, msg->second_segmentIDs, sizeof(uint16_t) * MAX_SEGMENTS_MSG);
}

int getNextCurrSegment(uint16_t *lock_sectors, uint8_t *next_sector, int trainNo, int segmentId) {
    int currId =  lock_sectors[segmentId];
    for (int i = 0; i < N_SEGMENTS; i++) {
        if ((next_sector[i] == (currId +1)) && (lock_sectors[i] == trainNo)) {
            uart_printf(CONSOLE, "train %d moving to segment %d", trainNo, i);
            return i;
        }
    }
    return -1;
}

void track_server_main() {
    int senderTid;
    struct msg_ts_server msg_received;

    RegisterAs(TS_SERVER_NAME);

    int clock = WhoIs(CLOCK_SERVER_NAME);
    int console = WhoIs(CONSOLE_SERVER_NAME);

    // data structures for keeping the locks
    uint16_t lock_sectors[N_SEGMENTS];
    uint8_t next_sector[N_SEGMENTS];
    memset(lock_sectors, 0, sizeof(uint16_t) * N_SEGMENTS);

    // data structure for blocking tids
    train_locking_structure train_data[N_TRNS];
    memset(train_data, 0, N_TRNS * sizeof(train_locking_structure));

    // Start three notifiers and safe them in the train structure
    for (int i = 0; i < N_TRNS; i++) {
        train_data[i].notifierTid = Create(P_NOTIF, track_server_timeout_notifier);
        Delay(clock, 10);
        Receive(&senderTid, (char *)&msg_received, sizeof(struct msg_ts_server));
    }

    for(;;) {
        Receive(&senderTid, (char *)&msg_received, sizeof(struct msg_ts_server));

        switch (msg_received.type) {
            case MSG_TS_ASK_SEGMENT_FREE: {
                // checks for a single segment
                reply(senderTid, (lock_sectors[msg_received.segmentIDs[0]] == 0));
                break;
            }

            case MSG_TS_REQUEST_SEGMENTS_ALL_TIMEOUT: {
                // first check if it can lock all segments
                int train_hash = trn_hash(msg_received.trainNo);
                bool success = lockAll_Safe(console, lock_sectors, msg_received.segmentIDs, msg_received.no_segments, msg_received.trainNo, next_sector, &train_data[train_hash]);

                if (success) {
                    train_data[train_hash].t_state = train_idle;
                    reply(senderTid, true);
                } else {
                    train_data[train_hash].all_segments_required = true;
                    train_data[train_hash].t_state = train_blocked_timeout;
                    train_data[train_hash].requesterTid = senderTid;

                    // copy the rest
                    copyTrainData(&train_data[train_hash], &msg_received);

                    // start notifier
                    replyNotifier(train_data[train_hash].notifierTid, train_data[train_hash].trainNo, train_data[train_hash].timeout);
                }
                break;
            }

            case MSG_TS_REQUEST_SEGMENTS_ALL_NO_TIMEOUT: {
                int train_hash = trn_hash(msg_received.trainNo);
                bool success = lockAll_Safe(console, lock_sectors, msg_received.segmentIDs, msg_received.no_segments, msg_received.trainNo, next_sector, &train_data[train_hash]);
                if (success) {
                    train_data[train_hash].t_state = train_idle;
                    reply(senderTid, true);
                } else {
                    // copy the train data in storage and on next free check if they can be deqeued
                    train_data[train_hash].all_segments_required = true;
                    train_data[train_hash].t_state = train_blocked;
                    train_data[train_hash].requesterTid = senderTid;

                    // copy the rest
                    copyTrainData(&train_data[train_hash], &msg_received);
                }
                break;
            }

            case MSG_TS_REQUEST_SEGMENTS_ONE_TIMEOUT: {
                int train_hash = trn_hash(msg_received.trainNo);
                int locked_segmentId = lockOne_Safe(console, lock_sectors, msg_received.segmentIDs, msg_received.no_segments, msg_received.trainNo, next_sector, &train_data[train_hash]);
                if (locked_segmentId != -1) {
                    train_data[train_hash].t_state = train_idle;
                    replySegment(senderTid, locked_segmentId);
                } else {
                    int train_hash = trn_hash(msg_received.trainNo);
                    train_data[train_hash].all_segments_required = false;
                    train_data[train_hash].t_state = train_blocked_timeout;
                    train_data[train_hash].requesterTid = senderTid;

                    // copy the rest
                    copyTrainData(&train_data[train_hash], &msg_received);

                    // start notifier
                    replyNotifier(train_data[train_hash].notifierTid, train_data[train_hash].trainNo, train_data[train_hash].timeout);
                }
                break;
            }

            case MSG_TS_REQUEST_SEGMENTS_ONE_NO_TIMEOUT: {
                int train_hash = trn_hash(msg_received.trainNo);
                int locked_segmentId = lockOne_Safe(console, lock_sectors, msg_received.segmentIDs, msg_received.no_segments, msg_received.trainNo, next_sector, &train_data[train_hash]);
                if (locked_segmentId != -1) {
                    train_data[train_hash].t_state = train_idle;
                    replySegment(senderTid, locked_segmentId);
                } else {
                    // copy the train data in storage and on next free check if they can be deqeued
                    train_data[train_hash].all_segments_required = false;
                    train_data[train_hash].t_state = train_blocked;
                    train_data[train_hash].requesterTid = senderTid;

                    // copy the rest
                    copyTrainData(&train_data[train_hash], &msg_received);
                }
                break;
            }

            case MSG_TS_REQUEST_SEGMENTS_AND_OR_TIMEOUT: {
                // first try to lock the first segment
                int train_hash = trn_hash(msg_received.trainNo);

                bool success = lockAll_Safe(console, lock_sectors, msg_received.segmentIDs, msg_received.no_segments, msg_received.trainNo, next_sector, &train_data[train_hash]);
                if (success) {
                    // reply that the first one was successfull
                    train_data[train_hash].t_state = train_idle;
                    replySegment(senderTid, 0);
                } else {
                    // try to lock the second one
                    success = lockAll_Safe(console, lock_sectors, msg_received.second_segmentIDs, msg_received.second_no_segments, msg_received.trainNo, next_sector, &train_data[train_hash]);
                    if (success) {
                        // reply the second on
                        train_data[train_hash].t_state = train_idle;
                        replySegment(senderTid, 1);
                    } else {
                        // putting them into memory and starting timeout
                        train_data[train_hash].all_segments_required = true;
                        train_data[train_hash].t_state = train_blocked_timeout_two;
                        train_data[train_hash].requesterTid = senderTid;

                        copyTrainData(&train_data[train_hash], &msg_received);

                        // start notifier
                        replyNotifier(train_data[train_hash].notifierTid, train_data[train_hash].trainNo, train_data[train_hash].timeout);
                    }
                }
                break;
            }

            case MSG_TS_REQUEST_SEGMENTS_AND_OR_NO_TIMEOUT: {
                int train_hash = trn_hash(msg_received.trainNo);
                bool success = lockAll_Safe(console, lock_sectors, msg_received.segmentIDs, msg_received.no_segments, msg_received.trainNo, next_sector, &train_data[train_hash]);
                if (success) {
                    train_data[train_hash].t_state = train_idle;
                    replySegment(senderTid, 0);
                } else {
                    success = lockAll_Safe(console, lock_sectors, msg_received.second_segmentIDs, msg_received.second_no_segments, msg_received.trainNo, next_sector, &train_data[train_hash]);
                    if (success) {
                        // reply the second on
                        train_data[train_hash].t_state = train_idle;
                        replySegment(senderTid, 1);
                    } else {
                        // putting them into memory and starting timeout
                        train_data[train_hash].all_segments_required = true;
                        train_data[train_hash].t_state = train_blocked_two;
                        train_data[train_hash].requesterTid = senderTid;

                        copyTrainData(&train_data[train_hash], &msg_received);
                    }
                }
                break;
            }


            case MSG_TS_FREE_SEGMENTS: {
                int train_hash = trn_hash(msg_received.trainNo);
                // go through the entries to be freed
                for (int i = 0; i < msg_received.no_segments; i++) {
                    int index = msg_received.segmentIDs[i];
                    if (lock_sectors[index] == msg_received.trainNo) {
                        lock_sectors[index] = 0;
                        update_segment(console, index, 0);

                        // update the trains curr segment
                        train_data[train_hash].curr_index = getNextCurrSegment(lock_sectors, next_sector, msg_received.trainNo, index);
                        next_sector[index] = 0;
                    } else {
                        ULOG("sector was not locked for train %d", msg_received.trainNo);
                    }
                }

                // reply to the train
                reply(senderTid, true);

                // go through the list of trains and check if some train is blocked
                for(int i = 0; i < N_TRNS; i++) {
                    if (train_data[i].trainNo != msg_received.trainNo && train_data[i].t_state == train_blocked) {
                        // try to assign the locks to it
                        if (train_data[i].all_segments_required) {
                            bool success = lockAll_Safe(console, lock_sectors, train_data[i].segmentIDs, train_data[i].no_segments, train_data[i].trainNo, next_sector, &train_data[i]);

                            // only reply if success -> else ignore
                            if (success) {
                                train_data[i].t_state = train_idle;
                                reply(train_data[i].requesterTid, true);
                            }
                        } else {
                            int locked_segmentId = lockOne_Safe(console, lock_sectors, train_data[i].segmentIDs, train_data[i].no_segments, train_data[i].trainNo, next_sector, &train_data[i]);

                            // only reply if success
                            if (locked_segmentId != -1) {
                                train_data[i].t_state = train_idle;
                                replySegment(train_data[i].requesterTid, locked_segmentId);
                            }
                        }
                    } else if (train_data[i].trainNo != msg_received.trainNo && train_data[i].t_state == train_blocked_two) {

                        bool success = lockAll_Safe(console, lock_sectors, msg_received.segmentIDs, msg_received.no_segments, msg_received.trainNo, next_sector, &train_data[i]);

                        if (success) {
                            // reply that the first one was successfull
                            train_data[i].t_state = train_idle;
                            replySegment(train_data[i].requesterTid, 0);
                        } else {
                            // try to lock the second one
                            success = lockAll_Safe(console, lock_sectors, msg_received.second_segmentIDs, msg_received.second_no_segments, msg_received.trainNo, next_sector, &train_data[i]);
                            if (success) {
                                // reply the second on
                                train_data[i].t_state = train_idle;
                                replySegment(train_data[i].requesterTid, 1);
                            }
                            // do nothing if not yet possible to reply
                        }
                    }
                }
                break;
            }

            case MSG_TS_NOTIF: {
                if (msg_received.trainNo != 0) {
                    // handle timeout -> look at the train Number
                    int train_hash = trn_hash(msg_received.trainNo);

                    // ignore after reset
                    if (train_data[train_hash].t_state == train_idle) {
                        ULOG("Ignoring notifier after reseting the lock server\r\n");
                        break;
                    }

                    // special case
                    if (train_data[train_hash].t_state == train_blocked_timeout_two) {

                        bool success = lockAll_Safe(console, lock_sectors, msg_received.segmentIDs, msg_received.no_segments, msg_received.trainNo,  next_sector, &train_data[train_hash]);
                        if (success) {
                            // reply that the first one was successfull
                            replySegment(train_data[train_hash].requesterTid, 0);
                        } else {
                            // try to lock the second one
                            success = lockAll_Safe(console, lock_sectors, msg_received.second_segmentIDs, msg_received.second_no_segments, msg_received.trainNo,  next_sector, &train_data[train_hash]);
                            if (success) {
                                // reply the second on
                                replySegment(train_data[train_hash].requesterTid, 1);
                            } else {
                                reply(train_data[train_hash].requesterTid, false);
                            }
                        }

                    } else if (train_data[train_hash].all_segments_required) {
                        bool success = lockAll_Safe(console, lock_sectors, train_data[train_hash].segmentIDs, train_data[train_hash].no_segments, train_data[train_hash].trainNo, next_sector, &train_data[train_hash]);
                        reply(train_data[train_hash].requesterTid, success);
                    } else {
                        int lockedSegment = lockOne_Safe(console, lock_sectors, train_data[train_hash].segmentIDs, train_data[train_hash].no_segments, train_data[train_hash].trainNo, next_sector, &train_data[train_hash]);
                        replySegment(train_data[train_hash].requesterTid, lockedSegment);
                    }

                    train_data[train_hash].t_state = train_idle;
                } else {
                    // ignore startup message -> being able to reply
                    uart_printf(CONSOLE, "received startup msg from notifier of train \r\n");
                }
                break;
            }

            case MSG_TS_FREE_ALL: {
                // zero all segments for that train
                int train_id = msg_received.trainNo;

                for (int i = 0; i < N_SEGMENTS; i++) {
                    if (lock_sectors[i] == train_id) {
                        lock_sectors[i] = 0;
                        update_segment(console, i, train_id);
                    }
                }

                // set train to idle
                train_data[trn_hash(train_id)].t_state = train_idle;
                Reply(senderTid, NULL, 0);
                break;
            }

            case MSG_TS_TRAIN_REGISTER: {
                // lock that one segment 
                lock_sectors[msg_received.segmentIDs[0]] = msg_received.trainNo;
                update_segment(console, msg_received.segmentIDs[0], msg_received.trainNo);

                // update the curr segment of the train
                int train_hash = trn_hash(msg_received.trainNo);
                train_data[train_hash].curr_segment = msg_received.segmentIDs[0];

                // set the current position
                next_sector[msg_received.segmentIDs[0]] = 1;
                train_data[train_hash].curr_index = 2;

                uart_printf(CONSOLE, "registered train %d at sector %d", msg_received.trainNo, msg_received.segmentIDs[0]);

                Reply(senderTid, NULL, 0);
                break;
            }

            default: {
                replyError(senderTid);
                break;
            }
        }
    }
}

void track_server_timeout_notifier(void) {
    //int clock = WhoIs(CLOCK_SERVER_NAME);

    int clock = WhoIs(CLOCK_SERVER_NAME);
    int parent = MyParentTid();


    msg_ts_server msg;
    msg.type = MSG_TS_NOTIF;
    msg.trainNo = 0;

    // Send bootup message to server
    Send(parent, (char*)&msg, sizeof(msg_ts_server), (char*)&msg, sizeof(msg_ts_server));

    for (;;) {
        // Receive message on how long to wait
        Delay(clock, msg.timeout);

        Send(parent, (char*)&msg, sizeof(msg_ts_server), (char*)&msg, sizeof(msg_ts_server));
    }
}
