#ifndef __SPEED_DATA_H__
#define __SPEED_DATA_H__

#include "speed.h"

#define REVERSE_OFFSET 120 // mm
#define SHORT_MOVE_MIN 200000 // = 20cm
#define SHORT_MOVE_MAX 800000 // = 80cm

typedef struct speed_data {
    // all data in um/s / um/s2
    uint32_t velocity_data[N_TRNS][N_SPDS];
    int32_t acceleration_data[N_TRNS][N_SPDS][N_SPDS]; // start, stop speeds
    int32_t stopping_data[N_TRNS][N_SPDS]; // all from spd -> 0
    uint32_t short_moves[N_TRNS][N_SHORT_MOVES];
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

// estimate functions for acceleration without initial/final
// velocity over the distance/time between two sensors
// NOTE: target_final_spd / original_init_spd refer to at origin/target pt, not
// at short time/distance start point
// all expect short_dist as um

// get distance travelled during acceleration w/out final velocity over short_time
int64_t
estimate_initial_distance_acceleration(speed_data *data, uint16_t trn,
    uint8_t init_spd, uint8_t target_final_spd, uint32_t short_time);

// get time travelled during acceleration w/out final velocity over short_distance
uint32_t
estimate_initial_time_acceleration(speed_data *data, uint16_t trn,
    uint8_t init_spd, uint8_t target_final_spd, int32_t short_dist);

// get time travelled during acceleration w/out initial velocity over short_distance
uint32_t
estimate_final_time_acceleration(speed_data *data, uint16_t trn,
    uint8_t original_init_spd, uint8_t final_spd, int32_t short_dist);

// takes um and returns clock_ticks
int32_t get_short_move_delay(speed_data *data, uint16_t trn, uint32_t dist_goal);

#endif
