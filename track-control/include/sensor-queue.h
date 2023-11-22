#ifndef __SENSOR_QUEUE_H__
#define __SENSOR_QUEUE_H__

#include <stdbool.h>
#include <stdint.h>

#define N_SENSOR_MODULES 5
#define N_SENSORS 16
#define MAX_WAITING_PROCESSES 8

typedef struct sensor_data {
    uint16_t tid;
    uint8_t trn;
    int64_t expected_time; // ordering in the queue
    bool pos_rqst;
} sensor_data;

typedef struct sensor_queue_entry sensor_queue_entry;

struct sensor_queue_entry {
    sensor_data data;
    sensor_queue_entry *next;
};

typedef struct sensor_queue {
    sensor_queue_entry storage[MAX_WAITING_PROCESSES];
    sensor_queue_entry *freelist;

    sensor_queue_entry *sensors[N_SENSOR_MODULES][N_SENSORS];
} sensor_queue;

void sensor_queue_init(sensor_queue *sq);

// add a process to the waiting list
void sensor_queue_add_waiting_tid(sensor_queue *sq, uint16_t sensor_mod,
    uint16_t sensor_no, sensor_data *data);

// returns first waiting process via sensor
// see below for return codes
#define SENSOR_QUEUE_DONE 0
#define SENSOR_QUEUE_TIMEOUT -1
#define SENSOR_QUEUE_FOUND 1
#define SENSOR_QUEUE_EARLY 2

int sensor_queue_get_waiting_tid(sensor_queue *sq, uint16_t sensor_mod,
    uint16_t sensor_no, uint32_t activation_time, sensor_data *data);

void sensor_queue_adjust_waiting_tid(sensor_queue *sq, uint16_t sensor_mod,
    uint16_t sensor_no, uint16_t tid, int64_t expected_time);

void sensor_queue_free_train(sensor_queue *sq, uint16_t trn);

#endif
