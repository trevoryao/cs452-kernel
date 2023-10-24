#ifndef __SPEED_H__
#define __SPEED_H__

#include <stdint.h>

// space saving for the trns buf, ugly ugly ugly
#define N_TRNS 7
static const uint8_t ALL_TRNS[N_TRNS] = {1, 2, 24, 47, 54, 58, 77};

#define ERR_NO_TRN 50 // bad return code for getter f'ns

// maintains reverse and lights status
typedef struct speed_t {
    int8_t trns[N_TRNS]; // stores the speed of the indexed train (if negative, train is reversed)
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

#endif
