#ifndef __SPEED_H__
#define __SPEED_H__

#include <stdint.h>

// space saving for the trns buf, ugly ugly ugly
#define N_TRNS 3
#define N_SHORT_MOVES 18
static const uint8_t ALL_TRNS[N_TRNS] = {24, 58, 77};

static const uint32_t SHORT_MOVES_BASE = 150;
static const uint32_t SHORT_MOVES_STEPS = 25;

enum SPEEDS {
    SPD_STP = 0,
    SPD_VLO = 7,
    SPD_LO = 8,
    SPD_MED = 9,
    SPD_HI = 10,
    SPD_VHI = 11,
    N_SPDS = 6
};

#define ERR_NO_TRN 50 // bad return code for getter f'ns

// maintains reverse and lights status
typedef struct speed_t {
    int8_t trns[N_TRNS]; // stores the speed of the indexed train (if negative, train is reversed)
} speed_t;

void speed_t_init(speed_t *spd_t);

// hash f'n for trns array
// returns idx of given tr, or -1 otherwise
int8_t trn_hash(uint8_t tr);

int8_t trn_hash_reverse(uint8_t tr);

// n >= 0 (will maintain reverse status)
void speed_set(speed_t *spd_t, uint16_t n, uint16_t s);

uint8_t speed_display_get(speed_t *spd_t, uint16_t n); // return |speed| (w/out light mod)
uint8_t speed_get(speed_t *spd_t, uint16_t n); // returns |speed| (including light mod) -- internal only

int speed_is_rv(speed_t *spd_t, uint16_t n);
int speed_is_stopped(speed_t *spd_t, uint16_t n);

// toggles reverse on train
void speed_flip(speed_t *spd_t, uint16_t n);

#endif
