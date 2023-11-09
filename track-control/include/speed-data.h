#ifndef __SPEED_DATA_H__
#define __SPEED_DATA_H__

#include "speed.h"

#define REVERSE_OFFSET 120 // mm

typedef struct speed_data {
    // all data in um/s / um/s2
    uint32_t velocity_data[N_TRNS][N_SPDS];
    int32_t acceleration_data[N_TRNS][N_SPDS][N_SPDS]; // start, stop speeds
    int32_t stopping_data[N_TRNS][N_SPDS]; // all from spd -> 0
} speed_data;

void speed_data_init(speed_data *data);

// velocity data
int32_t get_velocity(speed_data *data, uint16_t n, uint16_t s); // um/s
int32_t get_acceleration(speed_data *data, uint16_t n, uint16_t speed1, uint16_t speed2);

// returns distance in um needed to accelerate trn from s1 -> s2
int32_t get_distance_from_acceleration(speed_data *data, uint16_t trn, uint16_t speed1, uint16_t speed2);

// returns time in clock server ticks (10ms ticks) needed to accelerate trn from s1 -> s2
int32_t get_time_from_acceleration(speed_data *data, uint16_t trn, uint16_t speed1, uint16_t speed2);

// returns distance in um needed to deaccelerate to base spd (7) and stop
int32_t get_stopping_distance(speed_data *data, uint16_t n, uint16_t spd);

// returns time taken to travel dist at current spd (expected distance in mm)
int64_t get_time_from_velocity(speed_data *data, uint16_t trn, int32_t dist, uint16_t s);

uint32_t get_time_from_velocity_um(speed_data *data, uint16_t trn, int32_t dist, uint16_t s);

// returns distance in um taken to travel t clock ticks at spd
int32_t get_distance_from_velocity(speed_data *data, uint16_t trn, int32_t ticks, uint16_t s);

int64_t get_distance_acceleration_estimate(speed_data *data, uint16_t trn, uint16_t speed1, uint16_t speed2, int32_t short_time);

#endif
