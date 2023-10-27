#include "sensor-worker.h"

#include "nameserver.h"
#include "uart-server.h"
#include "clock.h"
#include "msg.h"
#include "track-control-coordinator.h"

void sensor_worker_main() {
    // Messages for requests
    int latestTimestamp = 0;

    struct msg_tc_server msg_tc_send, msg_tc_received;


    // Get the required tids
    int clockTid = WhoIs(CLOCK_SERVER_NAME);
    int marklinTid = WhoIs(MARKLIN_SERVER_NAME);
    int tcTid = WhoIs(TC_SERVER_NAME);



    for (;;) {
        // Wait for the Queue to be empty
        WaitOutputEmpty(marklinTid);

        // Write sensor request
        Putc(marklinTid, S88_BASE + N_S88);

        // Get sensor data
        for (int i = 0; i < (N_S88 * 2); i++) {
            msg_tc_received.data[i] = Getc(marklinTid);
        }
        
        latestTimestamp = Time(clockTid);
        msg_tc_send.clockTick = latestTimestamp;

        // send data to the control server
        Send(tcTid, (char *)&msg_tc_send, sizeof(struct msg_tc_server), (char *)&msg_tc_received, sizeof(struct msg_tc_server));
    }
}
