#include "track-server.h"

#include "msg.h"
#include "task.h"
#include "nameserver.h"
#include "clock.h"
#include "track.h"
#include "track-data.h"
#include "track-node.h"
#include "util.h"
#include "rpi.h"

#include "deque.h"

extern track_node track[];
#define N_SEGMENTS 31


void replySuccess(int tid, track_node *node, uint16_t trainNo) {
    struct msg_ts_server msg_reply;
    msg_reply.requesterTid = tid;
    msg_reply.node = node;
    msg_reply.type = MSG_TS_REQUEST_SUCCESS;
    msg_reply.trainNo = trainNo;

    Reply(tid, (char *)&msg_reply, sizeof(struct msg_ts_server));
}

void replyFail(int tid, track_node *node, uint16_t trainNo) {
    struct msg_ts_server msg_reply;
    msg_reply.requesterTid = tid;
    msg_reply.node = node;
    msg_reply.type = MSG_TS_REQUEST_FAIL;
    msg_reply.trainNo = trainNo;

    Reply(tid, (char *)&msg_reply, sizeof(struct msg_ts_server));
}

/* TODOS:
*
*   - make waiting for locks blocking
*   - add support for entries and exits
*
*/
void track_server_main() {
    int senderTid;
    struct msg_ts_server msg_received;

    int process_lock_server = 0;

    RegisterAs(TS_SERVER_NAME);
    int consoleTid = WhoIs(CONSOLE_SERVER_NAME);
    int clockTid = WhoIs(CLOCK_SERVER_NAME);

    // data structures for keeping the locks
    uint16_t lock_sectors[N_SEGMENTS];

    memset(lock_sectors, 0, sizeof(uint16_t) * N_SEGMENTS);

    // data structure for blocking tids
    deque waiting_tid;
    deque_init(&waiting_tid, 3);

    for(;;) {
        Receive(&senderTid, (char *)&msg_received, sizeof(struct msg_ts_server));

        // respond for the message, if server is reachable
        if (msg_received.type == MSG_TS_SERVER_AVAILABLE) {
            // ask if server is available
            (process_lock_server == 0) ? replySuccess(senderTid, 0, msg_received.trainNo) : replyFail(senderTid, 0, msg_received.trainNo);
            continue;

        }  else if (msg_received.type == MSG_TS_ACQUIRE_SERVER_LOCK) {
            // You need to acquire lock before you can do any operations

            if (process_lock_server == 0) {
                process_lock_server = msg_received.trainNo;
                replySuccess(senderTid, NULL, msg_received.trainNo);
            } else {
                // put it into the waiting queue
                deque_push_back(&waiting_tid, senderTid);
            }


        } else if (process_lock_server == senderTid) {
            // sanity check that it is a sensor
            if (msg_received.node->type != NODE_SENSOR) {
                replyFail(senderTid, msg_received.node, msg_received.trainNo);
            }


            // allowed to do operations
            switch (msg_received.type)  {
                case MSG_TS_REQUEST_SEGMENT: {
                    // check if the direction is available
                    int segmentId = msg_received.node->segmentId;
                    
                    if (lock_sectors[segmentId] != 0 && lock_sectors[segmentId] != msg_received.trainNo) {
                        replyFail(senderTid, msg_received.node, msg_received.trainNo);
                    } else {
                        replySuccess(senderTid, msg_received.node, msg_received.trainNo);
                    }
                    
                    break;
                }

                case MSG_TS_FREE_SEGMENT: {
                    int segmentId = msg_received.node->segmentId;
                    // check if the lock is actually set
                    if (lock_sectors[segmentId] == 0 || lock_sectors[segmentId] == msg_received.trainNo) {
                        lock_sectors[segmentId] = 0;
                        replySuccess(senderTid, msg_received.node, msg_received.trainNo);
                    } else {
                        replyFail(senderTid, msg_received.node, msg_received.trainNo);
                    }

                    break;
                }
                
                case MSG_TS_FREE_SERVER_LOCK: {
                    replySuccess(senderTid, NULL, 0);
                    
                    // check if deque is empty
                    if (deque_empty(&waiting_tid)) {
                        process_lock_server = 0;
                    } else {
                        process_lock_server = deque_pop_front(&waiting_tid);
                        replySuccess(process_lock_server, NULL, 0);
                    }                    
                }
        
                default:
                    break;
            }   
        } else {
            replyFail(senderTid, NULL, 0);
        }

    }
}
