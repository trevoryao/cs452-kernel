#include "uassert.h"
#include "util.h"

#include "sensor-queue.h"
#include "speed-data.h"
#include "track-data.h"

speed_data spd_data;
track_node track[TRACK_MAX];

static void
print_module(sensor_queue *sq, int sensor_mod, int sensor_num) {
    struct sensor_queue_entry *head = sq->sensors[sensor_mod - 1][sensor_num - 1];

    uart_printf(CONSOLE, "Sensors waiting on %c%d\r\n",
        sensor_mod - 1 + 'A', sensor_num);

    while (head != NULL) {
        uart_printf(CONSOLE, "trn %d (%u) ", head->data.trn, head->data.expected_time);
        head = head->next;
    }

    uart_printf(CONSOLE, "\r\n***** No More Sensors *****\r\n");
}

void user_main(void) {
    speed_data_init(&spd_data);
    init_track_b(track);

    sensor_queue queue;
    sensor_queue_init(&queue);

    sensor_data data;

    data.tid = 10;
    data.trn = 77;
    data.expected_time = 1000;
    data.pos_rqst = true;
    sensor_queue_add_waiting_tid(&queue, 5, 16, &data);

    data.tid = 15;
    data.trn = 24;
    data.expected_time = 10;
    sensor_queue_add_waiting_tid(&queue, 5, 16, &data);

    data.tid = 10;
    data.trn = 77;
    data.expected_time = 500;
    sensor_queue_add_waiting_tid(&queue, 5, 16, &data);

    data.tid = 16;
    data.trn = 24;
    data.expected_time = 9999;
    sensor_queue_add_waiting_tid(&queue, 5, 16, &data);

    print_module(&queue, 5, 16);

    int ret;

    // within range
    memset(&data, 0, sizeof(sensor_data));
    ret = sensor_queue_get_waiting_tid(&queue, 5, 16, 510, &data);
    uassert(ret == SENSOR_QUEUE_TIMEOUT);
    uassert(data.tid == 15);
    uassert(data.trn == 24);
    uassert(data.expected_time == 10);
    uassert(data.pos_rqst == true);

    memset(&data, 0, sizeof(sensor_data));
    ret = sensor_queue_get_waiting_tid(&queue, 5, 16, 510, &data);
    uassert(ret == SENSOR_QUEUE_FOUND);
    uassert(data.tid == 10);
    uassert(data.trn == 77);
    uassert(data.expected_time == 500);
    uassert(data.pos_rqst == true);

    memset(&data, 0, sizeof(sensor_data));
    ret = sensor_queue_get_waiting_tid(&queue, 5, 16, 510, &data);
    uassert(ret == SENSOR_QUEUE_DONE);

    sensor_queue_free_train(&queue, 24);
    uassert(queue.sensors[4][15]->data.tid == 10);
    uassert(queue.sensors[4][15]->data.trn == 77);
    uassert(queue.sensors[4][15]->data.expected_time == 1000);
    uassert(queue.sensors[4][15]->data.pos_rqst == true);
    uassert(queue.sensors[4][15]->next == NULL);

    sensor_queue_free_train(&queue, 77);
    uassert(queue.sensors[4][15] == NULL);

    uart_printf(CONSOLE, "test completed\r\n");
}