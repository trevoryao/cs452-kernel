#ifndef __SENSOR_QUEUE_H__
#define __SENSOR_QUEUE_H__

#include <stdbool.h>
#include <stdint.h>

#include "controller-consts.h"

#define MAX_WAITING_PROCESSES 16

typedef struct speed_t speed_t;
typedef struct track_node track_node;

typedef struct sensor_data {
    uint8_t trn; // train at the front

    // information for determining when to return to caller
    // timestamps
    int64_t first_activation;
} sensor_data;

void sensor_data_init(sensor_data *data, uint8_t trn);

typedef struct sensor_queue_entry sensor_queue_entry;

struct sensor_queue_entry {
    sensor_data data;
    sensor_queue_entry *next;
};

/*
 * sensor queue does not block the snake
 */

typedef struct sensor_queue {
    sensor_queue_entry storage[MAX_WAITING_PROCESSES];
    sensor_queue_entry *freelist;

    sensor_queue_entry *sensors_front[NUM_SEN_PER_MOD * NUM_MOD];
    sensor_queue_entry *sensors_back[NUM_SEN_PER_MOD * NUM_MOD];

    speed_t *spd; // reference to the snake's speed structure
} sensor_queue;

void sensor_queue_init(sensor_queue *sq, speed_t *spd);

// must be called to set variables
void sensor_queue_wait(sensor_queue *sq, track_node *node, uint8_t trn);

// if the update was from the second train, it is popped off the queue,
// and the next one is updated with this information
// returns the time offset between the two trains, or -1 if
// (likely) from the same train
int64_t sensor_queue_update(sensor_queue *sq, track_node *node,
    uint32_t activation_time);

#endif
