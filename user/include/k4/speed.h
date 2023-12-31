#ifndef __SPEED_H__
#define __SPEED_H__

#include <stdint.h>

// space saving for the trns buf, ugly ugly ugly
#define N_TRNS 3
static const uint8_t ALL_TRNS[N_TRNS] = {24, 58, 77};

enum SPEEDS {
    SPD_STP = 0,
    SPD_LO = 7,
    SPD_MED = 9,
    SPD_HI = 11,
    N_SPDS = 4
};

#define ERR_NO_TRN 50 // bad return code for getter f'ns

// maintains reverse and lights status
typedef struct speed_t {
    int8_t trns[N_TRNS]; // stores the speed of the indexed train (if negative, train is reversed)

    // all data in um/s / um/s2
    uint32_t velocity_data[N_TRNS][N_SPDS];
    int32_t acceleration_data[N_TRNS][N_SPDS][N_SPDS]; // start, stop speeds
    uint32_t stopping_data[N_TRNS]; // all from 7 -> 0
} speed_t;

void speed_t_init(speed_t *spd_t);

// hash f'n for trns array
// returns idx of given tr, or -1 otherwise
int8_t trn_hash(uint8_t tr);

// n >= 0 (will maintain reverse status)
void speed_set(speed_t *spd_t, uint16_t n, uint16_t s);

uint8_t speed_display_get(speed_t *spd_t, uint16_t n); // return |speed| (w/out light mod)
uint8_t speed_get(speed_t *spd_t, uint16_t n); // returns |speed| (including light mod) -- internal only

int speed_is_rv(speed_t *spd_t, uint16_t n);
int speed_is_stopped(speed_t *spd_t, uint16_t n);

// toggles reverse on train
void speed_flip(speed_t *spd_t, uint16_t n);

// velocity data
int32_t get_velocity(speed_t *spd_t, uint16_t n, uint16_t s); // um/s

// as f'n of current speed
int32_t get_current_velocity(speed_t *spd_t, uint16_t n); // um/s
int32_t get_acceleration(speed_t *spd_t, uint16_t n, uint16_t s);
uint32_t get_stopping_distance(speed_t *spd_t, uint16_t n); // um

#endif
