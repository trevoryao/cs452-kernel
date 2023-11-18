#include "speed-data.h"

#include "math.h"
#include "util.h"
#include "uassert.h"
#include "speed.h"

#include "controller-consts.h"

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
    data->acceleration_data[0][2][3] = 135447; // 9 -> 11
    data->acceleration_data[0][3][2] = -55904; // 11 -> 9
    data->acceleration_data[0][0][2] = 99929;  // 0 -> 9


    // trn 58
    data->acceleration_data[1][0][1] = 82661;  // 0 -> 7
    data->acceleration_data[1][1][2] = 76213;  // 7 -> 9
    data->acceleration_data[1][2][1] = -39682; // 9 -> 7
    data->acceleration_data[1][1][3] = 115697; // 7 -> 11
    data->acceleration_data[1][3][1] = -80692; // 11 -> 7
    data->acceleration_data[1][2][3] = 107489; // 9 -> 11
    data->acceleration_data[1][3][2] = -44940; // 11 -> 9
    data->acceleration_data[1][0][2] = 88112;  // 0 -> 9


    // trn 77
    data->acceleration_data[2][0][1] = 61097;  // 0 -> 7
    data->acceleration_data[2][1][2] = 55562;  // 7 -> 9
    data->acceleration_data[2][2][1] = -32683; // 9 -> 7
    data->acceleration_data[2][1][3] = 81505;  // 7 -> 11
    data->acceleration_data[2][3][1] = -69310; // 11 -> 7
    data->acceleration_data[2][2][3] = 71978; // 9 -> 11
    data->acceleration_data[2][3][2] = -37873; // 11 -> 9
    data->acceleration_data[2][0][2] = 72385;  // 0 -> 9


    // stopping data
    // velocity data
    for (int i = 0; i < N_TRNS; ++i)
        memset(data->stopping_data[i], 0, sizeof(int32_t) * N_SPDS);

    // tr 24
    data->stopping_data[0][1] = 135168;
    data->stopping_data[0][2] = 307080;
    data->stopping_data[0][3] = 583030;

    // tr 58
    data->stopping_data[1][1] = 127966; // 7 -> 0
    data->stopping_data[1][2] = 279043; // 9 -> 0
    data->stopping_data[1][3] = 545819; // 11 -> 0

    // tr 77
    data->stopping_data[2][1] = 241126; // 7 -> 0
    data->stopping_data[2][2] = 438894; // 9 -> 0
    data->stopping_data[2][3] = 768428; // 11 -> 0


    // short moves data
    data->short_moves[2][0] = 185000;
    data->short_moves[2][1] = 235000;
    data->short_moves[2][2] = 270000;
    data->short_moves[2][3] = 350000;
    data->short_moves[2][4] = 390000;
    data->short_moves[2][5] = 400000;
    data->short_moves[2][6] = 440000;
    data->short_moves[2][7] = 490000;
    data->short_moves[2][8] = 520000;
    data->short_moves[2][9] = 570000;
    data->short_moves[2][10] = 620000;
    data->short_moves[2][11] = 710000;
    data->short_moves[2][12] = 750000;
    data->short_moves[2][13] = 800000;
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

// expects distance in mm
int64_t
get_time_from_velocity(speed_data *data, uint16_t trn, int32_t dist, uint16_t s) {
    return get_time_from_velocity_um(data, trn, dist * MM_TO_UM, s);
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

// get distance travelled during acceleration w/out final velocity over short_time
int64_t
estimate_initial_distance_acceleration(speed_data *data, uint16_t trn,
    uint8_t init_spd, uint8_t target_final_spd, uint32_t short_time) {
    /*
     * Formula: d = (v1 * t) + 1/2 * a * t^2
     */

    int32_t v1 = get_velocity(data, trn, init_spd);
    int32_t a = get_acceleration(data, trn, init_spd, target_final_spd);

    int64_t part1 = (v1 * (int64_t)short_time) * 100;
    int64_t part2 = (a * (int64_t)short_time * (int64_t)short_time) / 2;

    return (part1 + part2) / (100 * 100);
}

// get time travelled during acceleration w/out final velocity over short_distance
uint32_t
estimate_initial_time_acceleration(speed_data *data, uint16_t trn,
    uint8_t init_spd, uint8_t target_final_spd, int32_t short_dist) {
    /*
     * Formula: d = (v1 * t) + 1/2 * a * t^2
     * Rearrange via Quadratic Formula:
     *
     * t = (-v1 +- sqrt(v1^2 + 2ad)) / a
     */

    int64_t v1 = get_velocity(data, trn, init_spd);
    int64_t a = get_acceleration(data, trn, init_spd, target_final_spd);

    int64_t sqrt_comp = sqrt((v1 * v1) + (2 * a * short_dist));

    int64_t t1 = ((-v1 + sqrt_comp) * 100) / a;
    int64_t t2 = ((-v1 - sqrt_comp) * 100) / a;
    return (uint32_t)((t2 > 0) ? t2 : t1);
}

// get time travelled during acceleration w/out initial velocity over short_distance
uint32_t
estimate_final_time_acceleration(speed_data *data, uint16_t trn,
    uint8_t original_init_spd, uint8_t final_spd, int32_t short_dist) {
    /*
     * Formula: d = (v2 * t) - 1/2 * a * t^2        (note different from above)
     * Rearrange via Quadratic Formula:
     *
     * t = (v2 +- sqrt(v2^2 - 2ad)) / a
     */

    int64_t v2 = get_velocity(data, trn, final_spd);
    int64_t a = get_acceleration(data, trn, original_init_spd, final_spd);

    int64_t sqrt_comp = sqrt((v2 * v2) - (2 * a * short_dist));

    int64_t t1 = ((v2 + sqrt_comp) * 100) / a;
    int64_t t2 = ((v2 - sqrt_comp) * 100) / a;
    return (uint32_t)((t2 > 0) ? t2 : t1);
}

int32_t get_short_move_delay(speed_data *data, uint16_t trn, uint32_t dist_goal) {
    int train_hash = trn_hash(trn);
    // first get the right two time steps
    int i = 0;
    int32_t distance = data->short_moves[train_hash][i];
    while ((i < N_SHORT_MOVES) && (dist_goal > distance)) {
        i += 1;
        distance = data->short_moves[train_hash][i];
    }

    // sanity check
    if (i == 0 || dist_goal > data->short_moves[train_hash][i]) {
        ULOG("Distance out of range for short distance");
        return -1;
    }

    // interpolate between the two distances
    int point1 = SHORT_MOVES_BASE + (i-1) * SHORT_MOVES_STEPS;
    int point2 = SHORT_MOVES_BASE + i * SHORT_MOVES_STEPS;


    // slope
    int slope = (dist_goal - data->short_moves[train_hash][i-1]) * 100 / (data->short_moves[train_hash][i] - data->short_moves[train_hash][i-1]);

    return (SHORT_MOVES_STEPS * slope) / 100 + point1;
}
