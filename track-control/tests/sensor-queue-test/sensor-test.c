#include "deque.h"
#include "rpi.h"
#include "uassert.h"
#include "util.h"

#include "sensor-queue.h"
#include "controller-consts.h"
#include "routing.h"
#include "speed-data.h"
#include "track-data.h"

speed_data spd_data;
track_node track[TRACK_MAX];

void user_main(void) {
    speed_data_init(&spd_data);
    init_track_b(track);
    sensor_queue queue;

    sensor_queue_init(&queue);

    uart_printf(CONSOLE, "Queue init");

    bool empty = sensor_queue_empty(&queue, 1, 1);
    uart_printf(CONSOLE, "Queue should be empty for sensor 1 and mod 1: %d\r\n", empty);

    sensor_queue_add_waiting_tid(&queue, 1, 1, 66);
    empty = sensor_queue_empty(&queue, 1, 1);
    uart_printf(CONSOLE, "Queue should NOT be empty for sensor 1 and mod 1: %d\r\n", empty);

    uint16_t tid = sensor_queue_get_waiting_tid(&queue, 1, 1);
    uart_printf(CONSOLE, "Received TID: %d\r\n", tid);






}