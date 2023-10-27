#include "track-control-coordinator.h"

#include "msg.h"
#include "nameserver.h"

void track_control_coordinator_main() {
    // structs for reading & writing
    int senderTid;
    struct msg_tc_server msg_received, msg_send;


    // Register at Nameserver
    RegisterAs(TC_SERVER_NAME);


    for (;;) {
        Receive(&senderTid, (char *)&msg_received, sizeof(struct msg_tc_server));


        switch (msg_received.type) {
            case MSG_TC_PLAN_REQUEST: {
                /* code */
                break;
            }


            case MSG_TC_SENSOR_GET: {
                /* code */
                

                break;
            }

            case MSG_TC_SENSOR_PUT: {
                /* code */
                break;
            } 
        
            default:
                break;
        }
    }


}