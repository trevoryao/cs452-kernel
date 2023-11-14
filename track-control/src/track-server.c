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
#define N_SEGMENTS 34


void replySuccess(int tid, track_node *node) {
    struct msg_ts_server msg_reply;
    msg_reply.requesterTid = tid;
    msg_reply.node = node;
    msg_reply.type = MSG_TS_REQUEST_SUCCESS;

    Reply(tid, (char *)&msg_reply, sizeof(struct msg_ts_server));
}

void replyFail(int tid, track_node *node) {
    struct msg_ts_server msg_reply;
    msg_reply.requesterTid = tid;
    msg_reply.node = node;
    msg_reply.type = MSG_TS_REQUEST_FAIL;

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
    uint16_t amount_lock[N_SEGMENTS];
    int8_t direction_lock[N_SEGMENTS];

    memset(amount_lock, 0, sizeof(uint16_t) * N_SEGMENTS);
    memset(direction_lock, DIR_EMPTY, sizeof(int8_t) * N_SEGMENTS);

    // data structure for blocking tids
    deque waiting_tids;
    deque_init(&waiting_tids, 3);

    uart_printf(CONSOLE, "Segment 21 has direction %d\r\n", direction_lock[21]);


    for(;;) {
        Receive(&senderTid, (char *)&msg_received, sizeof(struct msg_ts_server));

        // respond for the message, if server is reachable
        if (msg_received.type == MSG_TS_SERVER_AVAILABLE) {
            // ask if server is available
            (process_lock_server == 0) ? replySuccess(senderTid, 0) : replyFail(senderTid, 0);
            continue;

        }  else if (msg_received.type == MSG_TS_ACQUIRE_SERVER_LOCK) {
            // You need to acquire lock before you can do any operations

            if (process_lock_server == 0) {
                process_lock_server = senderTid;
                replySuccess(senderTid, NULL);
            } else {
                // put it into the waiting queue
                deque_push_back(&waiting_tids, senderTid);
            }


        } else if (process_lock_server == senderTid) {
            // allowed to do operations
            switch (msg_received.type)  {
                case MSG_TS_REQUEST_SEGMENT: {
                    // check if the direction is available
                    enum direction_lock dirLock = msg_received.node->edge[msg_received.directionNode].dirSegment;
                    int segmentId = msg_received.node->edge[msg_received.directionNode].segmentId;

                    // one train only segment
                    if (dirLock == DIR_NONE) {
                        if (amount_lock[segmentId] == senderTid || amount_lock[segmentId] == 0) {
                            amount_lock[segmentId] = senderTid;
                            direction_lock[segmentId] = DIR_NONE;
                            replySuccess(senderTid, msg_received.node);
                        } else {
                            replyFail(senderTid, msg_received.node);
                        }
                        // normal locks
                    } else if (direction_lock[segmentId] == DIR_EMPTY || direction_lock[segmentId] == dirLock) {
                        direction_lock[segmentId] = dirLock;
                        amount_lock[segmentId] += 1;
                        replySuccess(senderTid, msg_received.node);
                    } else {
                        replyFail(senderTid, msg_received.node);
                    }
                    
                    break;
                }

                case MSG_TS_FREE_SEGMENT: {
                    enum direction_lock dirLock = msg_received.node->edge[msg_received.directionNode].dirSegment;
                    int segmentId = msg_received.node->edge[msg_received.directionNode].segmentId;
                    // check if the lock is actually set
                    if (dirLock == DIR_NONE) {
                        amount_lock[segmentId] = 0;
                        direction_lock[segmentId] = DIR_EMPTY;
                        replySuccess(senderTid, msg_received.node);
                    } else if (amount_lock[segmentId] != 0) {
                        // decrease lock and reply
                        amount_lock[segmentId] -= 1;
                        if (amount_lock[segmentId] == 0) {
                            direction_lock[segmentId] = DIR_EMPTY;
                        }

                        replySuccess(senderTid, msg_received.node);
                    } else {
                        replyFail(senderTid, msg_received.node);
                    }

                    break;
                }
                
                case MSG_TS_FREE_SERVER_LOCK: {
                    replySuccess(senderTid, NULL);
                    
                    // check if deque is empty
                    if (deque_empty(&waiting_tids)) {
                        process_lock_server = 0;
                    } else {
                        process_lock_server = deque_pop_front(&waiting_tids);
                    }                    
                }
        
                default:
                    break;
            }   
        } else {
            replyFail(senderTid, NULL);
        }

    }
}
