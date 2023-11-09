#include "speed.h"

// lib
#include "util.h"

#include "controller-consts.h"

void speed_t_init(speed_t *spd_t) {
    memset(spd_t->trns, 0, N_TRNS * sizeof(uint8_t)); // all trn spds set to 0 at start
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

int8_t trn_hash_reverse(uint8_t tr) {
    // fast, ugly (look away)
    switch (tr) {
        case 0: return 24;
        case 1: return 58;
        case 2: return 77;
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
