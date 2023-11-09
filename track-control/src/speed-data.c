#include "speed-data.h"

#include "util.h"
#include "uassert.h"

inline static uint8_t spd_hash(enum SPEEDS s) {
    if (s == 0) return 0;
    return (s - 5) >> 1; // div 2
}

void speed_data_init(speed_data *data) {
    // velocity data
    for (int i = 0; i < N_TRNS; ++i)
        memset(data->velocity_data[i], 0, sizeof(uint32_t) * N_SPDS);

    // trn 24
    data->velocity_data[0][1] = 162083; // spd 7 (LO)
    data->velocity_data[0][2] = 279550; // spd 9 (MED)
    data->velocity_data[0][3] = 407185; // spd 11 (HI)

    // trn 58
    data->velocity_data[1][1] = 144318; // spd 7 (LO)
    data->velocity_data[1][2] = 247857; // spd 9 (MED)
    data->velocity_data[1][3] = 366255; // spd 11 (HI)

    // trn 77
    data->velocity_data[2][1] = 181496; // spd 7 (LO)
    data->velocity_data[2][2] = 280784; // spd 9 (MED)
    data->velocity_data[2][3] = 403022; // spd 11 (HI)

    // acceleration data
    for (int i = 0; i < N_TRNS; ++i)
        for (int j = 0; j < N_SPDS; ++j)
            memset(data->acceleration_data[i][j], 0, sizeof(int32_t) * N_SPDS);

    // trn 24
    data->acceleration_data[0][0][1] = 83762;  // 0 -> 7
    data->acceleration_data[0][1][2] = 114075; // 7 -> 9
    data->acceleration_data[0][2][1] = -49906; // 9 -> 7
    data->acceleration_data[0][1][3] = 166944; // 7 -> 11
    data->acceleration_data[0][3][1] = -95610; // 11 -> 7

    // trn 58
    data->acceleration_data[1][0][1] = 82661;  // 0 -> 7
    data->acceleration_data[1][1][2] = 76213;  // 7 -> 9
    data->acceleration_data[1][2][1] = -39682; // 9 -> 7
    data->acceleration_data[1][1][3] = 115697; // 7 -> 11
    data->acceleration_data[1][3][1] = -80692; // 11 -> 7

    // trn 77
    data->acceleration_data[2][0][1] = 61097;  // 0 -> 7
    data->acceleration_data[2][1][2] = 55562;  // 7 -> 9
    data->acceleration_data[2][2][1] = -32683; // 9 -> 7
    data->acceleration_data[2][1][3] = 81505;  // 7 -> 11
    data->acceleration_data[2][3][1] = -69310; // 11 -> 7

    // stopping data
    // velocity data
    for (int i = 0; i < N_TRNS; ++i)
        memset(data->stopping_data[i], 0, sizeof(int32_t) * N_SPDS);

    // tr 24
    data->stopping_data[0][1] = 135168;

    // tr 58
    data->stopping_data[1][1] = 127966; // 7 -> 0
    data->stopping_data[1][2] = 279043; // 9 -> 0
    data->stopping_data[1][3] = 545819; // 11 -> 0

    // tr 77
    data->stopping_data[2][1] = 241126; // 7 -> 0
    data->stopping_data[2][2] = 438894; // 9 -> 0
    data->stopping_data[2][3] = 768428; // 11 -> 0
}

int32_t get_velocity(speed_data *data, uint16_t n, uint16_t s) {
    return data->velocity_data[trn_hash(n)][spd_hash(s)];
}

int32_t get_acceleration(speed_data *data, uint16_t n, uint16_t speed1, uint16_t speed2) {
    return data->acceleration_data[trn_hash(n)][spd_hash(speed1)][spd_hash(speed2)];
}

int32_t get_stopping_distance(speed_data *data, uint16_t n, uint16_t spd) {
    return data->stopping_data[trn_hash(n)][spd_hash(spd)];
}

int32_t get_distance_from_acceleration(speed_data *data, uint16_t trn, uint16_t speed1, uint16_t speed2) {
    if (speed1 == speed2) return 0;

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
    if (idx < 0) return 0;

    // in um/s2
    int64_t acceleration = get_acceleration(data, trn, speed1, speed2);
    // velocity in um/s
    uint64_t v1 = get_velocity(data, trn, speed1);
    uint64_t v2 = get_velocity(data, trn, speed2);

    uint64_t v1_squared = v1 * v1;
    uint64_t v2_squared = v2 * v2;

    uint64_t distance = (int64_t)(v2_squared - v1_squared) / (2 * acceleration);

    return (int32_t) distance;
}

int32_t get_time_from_acceleration(speed_data *data, uint16_t trn, uint16_t speed1, uint16_t speed2) {
    if (speed1 == speed2) return 0;

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


    int64_t acceleration = get_acceleration(data, trn, speed1, speed2);
    int32_t v1 = get_velocity(data, trn, speed1);
    int32_t v2 = get_velocity(data, trn, speed2);

    int32_t t_clock_ticks = 100 * (v2 - v1);
    return t_clock_ticks / acceleration;
}


int64_t get_distance_acceleration_estimate(speed_data *data, uint16_t trn, uint16_t speed1, uint16_t speed2, int32_t short_time) {
    int64_t acceleration = get_acceleration(data, trn, speed1, speed2);
    if (acceleration == 0) return 0;

    // Get the initial velocity
    int32_t v1 = get_velocity(data, trn, speed1);

    int64_t distance_constant = v1 * short_time / 100;
    int64_t distance_speed_up = short_time * short_time / 1000;
    int64_t result = distance_constant + (distance_speed_up * acceleration) / 20;
    
    uart_printf(CONSOLE, "v1 %d, result %d, time: %d\r\n", v1, distance_speed_up, result, short_time);
    return result;
}

// expects distance in um
int64_t get_time_from_velocity(speed_data *data, uint16_t trn, int32_t dist, uint16_t s) {
    /*
    *
    *   Formula: t_clock_ticks = 100 * d / v
    *
    */
   int64_t velocity = get_velocity(data, trn, s);
   int64_t distance_in_um = dist * 1000;

   return 100 * distance_in_um / velocity;
}

uint32_t get_time_from_velocity_um(speed_data *data, uint16_t trn, int32_t dist, uint16_t s) {
    /*
    *
    *   Formula: t_clock_ticks = 100 * d / v
    *
    */
    int32_t velocity = get_velocity(data, trn, s);

    int64_t nominator = 100 * dist;
    int64_t result = nominator / velocity;

    return result;

}


int32_t get_distance_from_velocity(speed_data *data, uint16_t trn, int32_t ticks, uint16_t s) {
    /*
    *
    *   Formula: d = (ticks * v) / 100
    *
    */
   int32_t velocity = get_velocity(data, trn, s);

   return (ticks * velocity) / 100;
}
