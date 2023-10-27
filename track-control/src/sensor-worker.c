#include "sensor-worker.h"

#include "nameserver.h"
#include "uart-server.h"
#include "clock.h"

void sensor_worker_main() {
    // Messages for requests
    int latestTimestamp = 0;
    char sensorData[10];

    // Get the required tids
    int clockTid = WhoIs(CLOCK_SERVER_NAME);
    int marklinTid = WhoIs(MARKLIN_SERVER_NAME);



    for (;;) {
        // Wait for the Queue to be empty
        WaitOutputEmpty(marklinTid);

        // Write sensor request
        Putc(marklinTid, S88_BASE + N_S88);

        // Get sensor data
        for (int i = 0; i < (N_S88 * 2); i++) {
            sensorData[i] = Getc(marklinTid);
        }
        
        latestTimestamp = Time(clockTid);

        // send data to the control server
        // Send(...)
    }
}
