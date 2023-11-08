#ifndef __POSITION_H__
#define __POSITION_H__

#include <stdint.h>
#include "speed-data.h"

enum ACC_SPEED {
   STOPPED,
   CONSTANT_SPEED,
   STOPPING_TO_ZERO,
   ACCELERATING,
   DEACCELERATING
};

typedef struct trn_position {
    int8_t current_train_speed[N_TRNS];

    uint32_t distance_to_next_sensor[N_TRNS];
    uint32_t distance_to_next_next_sensor[N_TRNS];

    uint32_t last_timestamp[N_TRNS];

    uint16_t monitor_tid;
    uint16_t clock_tid;
} trn_position;

// Functions to calculate the position of the train
void trn_position_init(trn_position *trn_pos);

// update the actual train position -> prints difference btw. expected & actual
void trn_position_reached_next_sensor(trn_position *trn_pos, int8_t trainNo);

// updates the next expected sensor -> calculates the expected timestamp at next sensor
void trn_position_update_next_expected_pos(trn_position *trn_pos, int8_t trainNo, uint32_t next_distance);

// updates the train speed & recalulates the expected arriving time
void trn_position_update_train_speed(trn_position *trn_pos, int8_t trainNo, int8_t newSpeed);

#endif
