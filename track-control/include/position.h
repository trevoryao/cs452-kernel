#ifndef __SPEED_H__
#define __SPEED_H__

#include <stdint.h>

// space saving for the trns buf, ugly ugly ugly
#define N_TRNS 3

enum ACC_SPEED {
   CONSTANT_SPEED,
   STOPPING_TO_ZERO, 
   ACCELERATING,
   DEACCELERATING
};

typedef struct trn_position {
    enum ACC_SPEED train_state[N_TRNS];

    int8_t last_seen_sensor_mod[N_TRNS];
    int8_t last_seen_sensor_no[N_TRNS];

    int8_t next_expected_sensor_mod[N_TRNS];
    int8_t next_expected_sensor_no[N_TRNS];

    uint32_t last_seen_clock_tick[N_TRNS];
    uint32_t next_expected_clock_tick[N_TRNS];
} trn_position;

// Functions to calculate the position of the train
void trn_position_init(trn_position *trn_pos);

// update the actual train position -> prints difference btw. expected & actual
void trn_position_update_train_pos(int8_t trainNo, int8_t sensor_mod, int8_t sensor_no, uint32_t timestamp);

// updates the next expected sensor -> calculates the expected timestamp at next sensor
void trn_position_update_next_expected_pos(int8_t trainNo,  int8_t sensor_mod, int8_t sensor_no);

// updates the train speed & recalulates the expected arriving time
void trn_position_update_train_speed(int8_t trainNo, int8_t oldSpeed, int8_t newSpeed);


#endif
