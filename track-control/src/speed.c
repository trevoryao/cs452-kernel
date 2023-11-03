#include "speed.h"

// lib
#include "util.h"

#include "controller-consts.h"

inline static uint8_t spd_hash(enum SPEEDS s) {
    if (s == 0) return 0;
    return (s - 5) >> 1; // div 2
}

void speed_t_init(speed_t *spd_t) {
    memset(spd_t->trns, 0, N_TRNS * sizeof(uint8_t)); // all trn spds set to 0 at start

    // velocity data
    for (int i = 0; i < N_TRNS; ++i)
        memset(spd_t->velocity_data[i], 0, sizeof(uint32_t) * N_SPDS);

    // trn 24
    spd_t->velocity_data[0][1] = 162083; // spd 7 (LO)
    spd_t->velocity_data[0][2] = 279550; // spd 9 (MED)
    spd_t->velocity_data[0][3] = 407185; // spd 11 (HI)

    // trn 58
    spd_t->velocity_data[1][1] = 144318; // spd 7 (LO)
    spd_t->velocity_data[1][2] = 247857; // spd 9 (MED)
    spd_t->velocity_data[1][3] = 366255; // spd 11 (HI)

    // trn 77
    spd_t->velocity_data[2][1] = 181496; // spd 7 (LO)
    spd_t->velocity_data[2][2] = 280784; // spd 9 (MED)
    spd_t->velocity_data[2][3] = 403022; // spd 11 (HI)

    // acceleration data
    for (int i = 0; i < N_TRNS; ++i)
        for (int j = 0; j < N_SPDS; ++j)
            memset(spd_t->acceleration_data[i][j], 0, sizeof(int32_t) * N_SPDS);

    // trn 24
    spd_t->acceleration_data[0][0][1] = 83762;  // 0 -> 7
    spd_t->acceleration_data[0][1][2] = 114075; // 7 -> 9
    spd_t->acceleration_data[0][2][1] = -49906; // 9 -> 7
    spd_t->acceleration_data[0][1][3] = 166944; // 7 -> 11
    spd_t->acceleration_data[0][3][1] = -95610; // 11 -> 7

    // trn 58
    spd_t->acceleration_data[1][0][1] = 82661;  // 0 -> 7
    spd_t->acceleration_data[1][1][2] = 76213;  // 7 -> 9
    spd_t->acceleration_data[1][2][1] = -39682; // 9 -> 7
    spd_t->acceleration_data[1][1][3] = 115697; // 7 -> 11
    spd_t->acceleration_data[1][3][1] = -80692; // 11 -> 7

    // trn 77
    spd_t->acceleration_data[2][0][1] = 61097;  // 0 -> 7
    spd_t->acceleration_data[2][1][2] = 55562;  // 7 -> 9
    spd_t->acceleration_data[2][2][1] = -32683; // 9 -> 7
    spd_t->acceleration_data[2][1][3] = 81505;  // 7 -> 11
    spd_t->acceleration_data[2][3][1] = -69310; // 11 -> 7

    // stopping data
    spd_t->stopping_data[0] = 135168; // tr 24
    spd_t->stopping_data[1] = 127966; // tr 58
    spd_t->stopping_data[2] = 226606; // tr 77
}

int8_t trn_hash(uint8_t tr) {
    // fast, ugly (look away)
    switch (tr) {
        case 24: return 0;
        case 58: return 1;
        case 77: return 2;
        default: return -1;
    }
}

void speed_set(speed_t *spd_t, uint16_t n, uint16_t s) {
    int8_t idx = trn_hash(n);
    if (idx < 0) return;

    spd_t->trns[idx] = ((spd_t->trns[idx] < 0) ? -1 : 1) * s; // keep existing reverse status
}

uint8_t speed_display_get(speed_t *spd_t, uint16_t n) {
    int8_t idx = trn_hash(n);
    if (idx < 0) return ERR_NO_TRN;

    uint8_t abs_spd = speed_get(spd_t, n);

    // strip light modifier
    if (abs_spd >= LIGHTS) return abs_spd - LIGHTS;
    return abs_spd;
}

uint8_t speed_get(speed_t *spd_t, uint16_t n) {
    int8_t idx = trn_hash(n);
    if (idx < 0) return ERR_NO_TRN;

    return (speed_is_rv(spd_t, n) ? -1 : 1) * spd_t->trns[idx]; // abs
}

int speed_is_stopped(speed_t *spd_t, uint16_t n) {
    int8_t idx = trn_hash(n);
    if (idx < 0) return 0;

    return spd_t->trns[idx] == 0 || spd_t->trns[idx] == LIGHTS; // light mod
}

int speed_is_rv(speed_t *spd_t, uint16_t n) {
    int8_t idx = trn_hash(n);
    if (idx < 0) return 0;

    return spd_t->trns[idx] < 0;
}

void speed_flip(struct speed_t *spd_t, uint16_t n) {
    int8_t idx = trn_hash(n);
    if (idx < 0) return;

    spd_t->trns[idx] = -1 * spd_t->trns[idx];
}

int32_t get_current_velocity(speed_t *spd_t, uint16_t n) {
    return get_velocity(spd_t, n, speed_get(spd_t, n));
}

int32_t get_velocity(speed_t *spd_t, uint16_t n, uint16_t s) {
    return spd_t->velocity_data[trn_hash(n)][spd_hash(s)];
}

int32_t get_acceleration(speed_t *spd_t, uint16_t n, uint16_t s) {
    return spd_t->acceleration_data[trn_hash(n)][spd_hash(speed_get(spd_t, n))][spd_hash(s)];
}

int32_t get_acceleration_general(speed_t *spd_t, uint16_t n, uint16_t speed1, uint16_t speed2) {
    return spd_t->acceleration_data[trn_hash(n)][spd_hash(speed1)][spd_hash(speed2)];
}



uint32_t get_stopping_distance(speed_t *spd_t, uint16_t n) {
    return spd_t->stopping_data[trn_hash(n)];
}

int32_t get_distance_from_acceleration(speed_t *spd_t, uint16_t trn, uint16_t speed1, uint16_t speed2) {
    /*
    *   We assume a linear acceleration
    *   Basic kinematic formula
    *   v^2 = u^2 + 2 * a * d 
    *   
    *   with:
    *       - v^2 [(um/s)^2]     = squared final velocity (here v2)
    *       - u^2 [(um/s)^2]     = squared inital velocity (here v1)
    *       - a   [um/(s^2)]     = acceleration
    *       - d   [um]           = distance traveled
    *
    *   To calculate the distance
    * 
    *   d = 0.5 * (v^2 - u^2) / a 
    * 
    */
    
    int8_t idx = trn_hash(trn);
    if (idx < 0) return;

    // in um/s2
    int64_t acceleration = get_acceleration_general(spd_t, trn, speed1, speed2);
    // velocity in um/s
    int32_t v1 = get_velocity(spd_t, trn, speed1);
    int32_t v2 = get_velocity(spd_t, trn, speed2);

    int64_t v1_squared = v1 * v1;
    int64_t v2_squared = v2 * v2;

    int64_t distance = (v2_squared - v1_squared) / (2 * acceleration);

    return (int32_t) distance;
}

int32_t get_time_from_acceleration(speed_t *spd_t, uint16_t trn, uint16_t speed1, uint16_t speed2) {
    /*
    *   We again assume linear acceleration
    *   Basic formula: 
    *   v = u + t * a
    *   
    *   with:
    *       - v   [(um/s)]       = final velocity (here v2)
    *       - u   [(um/s)]       = inital velocity (here v1)
    *       - a   [um/s]         = acceleration
    *       - t   [s]            = time returned
    * 
    *  t_seconds = (v - u) / a
    *  t_clock_ticks = 100 * (v - u) / a
    */


    int64_t acceleration = get_acceleration_general(spd_t, trn, speed1, speed2);
    int32_t v1 = get_velocity(spd_t, trn, speed1);
    int32_t v2 = get_velocity(spd_t, trn, speed2);


    int32_t t_clock_ticks = 100 * (v2 - v1);
    return t_clock_ticks / acceleration;
}

int32_t get_time_from_velocity(speed_t *spd_t, uint16_t trn, int32_t dist) {
    /*
    *
    *   Formula: t_clock_ticks = 100 * d / v
    * 
    */
   int32_t velocity = get_current_velocity(spd_t, trn);
   int32_t distance_in_um = dist * 1000;

   return 100 * distance_in_um / velocity;
}