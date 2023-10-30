#ifndef __SENSOR_QUEUE_H__
#define __SENSOR_QUEUE_H__

#include <stdbool.h>
#include <stdint.h>

#define N_SENSOR_MODULES 5
#define N_SENSORS 16
#define MAX_WAITING_PROCESSES 64

typedef struct sensor_queue_entry sensor_queue_entry;

typedef struct sensor_queue_entry {
    // assigning Ids missing here
    uint16_t tid;
    sensor_queue_entry *next; 

} sensor_queue_entry;

typedef struct sensor_queue {
    sensor_queue_entry storage[MAX_WAITING_PROCESSES];
    sensor_queue_entry *freelist;

    sensor_queue_entry *sensors[N_SENSOR_MODULES][N_SENSORS];
} sensor_queue;


void sensor_queue_init(sensor_queue *sq);

// returns if any task is waiting for this specific module
bool sensor_queue_empty(sensor_queue *sq, uint16_t sensor_mod, uint16_t sensor_no);

// returns first waiting tid
uint16_t sensor_queue_get_waiting_tid(sensor_queue *sq, uint16_t sensor_mod, uint16_t sensor_no);

// add a process to the waiting list
void sensor_queue_add_waiting_tid(sensor_queue *sq, uint16_t sensor_mod, uint16_t sensor_no, uint16_t tid);

#endif

