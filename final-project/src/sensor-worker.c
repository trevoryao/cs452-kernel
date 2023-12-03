#include "sensor-worker.h"

#include "controller-consts.h"
#include "nameserver.h"
#include "uart-server.h"
#include "clock.h"
#include "msg.h"
#include "task.h"
#include "track-data.h"
#include "rpi.h"
#include "snake.h"

#define N_SENSOR 8

extern track_node track[];

static void
parseAndReply(char data, int no_of_byte, int snake_tid, uint32_t activation_ticks) {
    int sensor_mod = (no_of_byte / 2) + 1;
    int mod_round = no_of_byte % 2;

    for (uint8_t sen_bit = 1; sen_bit <= N_SENSOR; ++sen_bit) {
        if (((data & (1 << (N_SENSOR - sen_bit))) >> (N_SENSOR - sen_bit)) == 0x01) { // bit matches?
            int16_t sensor_no = sen_bit + (mod_round * N_SENSOR);

            // send to server
            snake_server_sensor_triggered(snake_tid,
                &track[TRACK_NUM_FROM_SENSOR(sensor_mod, sensor_no)],
                activation_ticks);
        }
    }
}

void sensor_worker_main() {
    // Get the required tids
    int marklinTid = WhoIs(MARKLIN_SERVER_NAME);
    int snakeTid = WhoIs(SNAKE_NAME);
    int clockTid = WhoIs(CLOCK_SERVER_NAME);

    uint32_t rqst_time;

    for (;;) {
        // Wait for the Queue to be empty
        WaitOutputEmpty(marklinTid);

        // Write sensor request
        Putc(marklinTid, S88_BASE + N_S88);
        rqst_time = Time(clockTid); // estimate

        // Get sensor data
        for (int i = 0; i < (N_S88 * 2); i++) {
            char data = Getc(marklinTid);

            if (data != 0x0) {
                //uart_printf(CONSOLE, "received data");
                parseAndReply(data, i, snakeTid, rqst_time);
            }
        }
    }
}
