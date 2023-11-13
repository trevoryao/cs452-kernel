#include "track-server.h"

#include "msg.h"
#include "task.h"
#include "nameserver.h"
#include "clock.h"
#include "track.h"


void replySuccess(int tid, uint8_t segmentID) {
    struct msg_ts_server msg_reply;
    msg_reply.requesterTid = tid;
    msg_reply.segmentID = segmentID;
    msg_reply.type = MSG_TS_REQUEST_SUCCESS;

    Reply(tid, (char *)&msg_reply, sizeof(struct msg_ts_server));
}

void replyFail(int tid, uint8_t segmentID) {
    struct msg_ts_server msg_reply;
    msg_reply.requesterTid = tid;
    msg_reply.segmentID = segmentID;
    msg_reply.type = MSG_TS_REQUEST_FAIL;

    Reply(tid, (char *)&msg_reply, sizeof(struct msg_ts_server));
}


void track_server_main() {
    int senderTid;
    struct msg_ts_server msg_received;



    RegisterAs(TS_SERVER_NAME);



    int consoleTid = WhoIs(CONSOLE_SERVER_NAME);
    int clockTid = WhoIs(CLOCK_SERVER_NAME);


    for(;;) {
        Receive(&senderTid, (char *)&msg_received, sizeof(struct msg_ts_server));

        switch (msg_received.type)  {
            case MSG_TS_REQUEST_SEGMENT: {
                break;
            }

            case MSG_TS_FREE_SEGMENT: {
                break;
            }
                
        
            default:
                break;
        }
    }


}