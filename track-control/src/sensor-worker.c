#include "sensor-worker.h"

#include "nameserver.h"
#include "uart-server.h"
#include "clock.h"
#include "msg.h"
#include "track-control-coordinator.h"
#include "task.h"
#include "track-control.h"
#include "rpi.h"

#define N_SENSOR 8

void parseAndReply(char data, int no_of_byte, int tcTid, int ownTid) {
    int sensor_mod = (no_of_byte / 2) + 1;
    int mod_round = no_of_byte % 2;

    for (uint8_t sen_bit = 1; sen_bit <= N_SENSOR; ++sen_bit) {
        //uart_printf(CONSOLE, "parsing data \r\n");
        if (((data & (1 << (N_SENSOR - sen_bit))) >> (N_SENSOR - sen_bit)) == 0x01) { // bit matches?
            int16_t sensor_no = sen_bit + (mod_round * N_SENSOR);

            //send to server
            //uart_printf(CONSOLE, "Received sensor_mod %d, sensor_no %d \r\n", sensor_mod, sensor_no);
            track_control_put_sensor(tcTid, sensor_mod, sensor_no);
        }
    }
}


void sensor_worker_main() {
    // Get the required tids
    int marklinTid = WhoIs(MARKLIN_SERVER_NAME);
    int clockTid = WhoIs(CLOCK_SERVER_NAME);
    int tcTid = WhoIs(TC_SERVER_NAME);
    int ownTid = MyTid();

    for (;;) {
        // Wait for the Queue to be empty
        WaitOutputEmpty(marklinTid);

        // Write sensor request
        Putc(marklinTid, S88_BASE + N_S88);

        // Get sensor data
        for (int i = 0; i < (N_S88 * 2); i++) {
            char data = Getc(marklinTid);

            if (data != 0x0) {
                //uart_printf(CONSOLE, "received data");
                parseAndReply(data, i, tcTid, ownTid);
            }
        }
       
    }
}
