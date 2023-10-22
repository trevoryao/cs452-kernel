#include "k4/track.h"

#include "uart-server.h"

#include "k4/controller-consts.h"
#include "k4/speed.h"

// byte cmds
#define SOL_OFF      32
#define CMD_GO_BYTE  96
#define CMD_STP_BYTE 97

#define CMD_BYTES    2

static void train_mod_speed_unchecked(uint16_t tid, uint16_t n, uint16_t s) {
    char cmds[CMD_BYTES] = {s == 0 ? ZERO_BYTE : s, n};
    Putl(tid, cmds, CMD_BYTES);
}

void train_mod_speed(uint16_t tid, struct speed_t *spd_t, uint16_t n, uint16_t s) {
    if (s == SP_REVERSE || s == (SP_REVERSE + LIGHTS)) return; // prevent reverse

    speed_set(spd_t, n, s); // store state
    train_mod_speed_unchecked(tid, n, s); // send lights
}

void train_reverse_start(uint16_t tid, speed_t *spd_t, uint16_t n) {
    train_mod_speed_unchecked(tid, n, SP_STOP); // doesn't modify spd_t

    speed_flip(spd_t, n); // store state
    // reverse task handles wait for us
}

void train_reverse_end(uint16_t tid, uint16_t s, uint16_t n) {
    train_mod_speed_unchecked(tid, n, SP_REVERSE);      // reverse
    train_mod_speed_unchecked(tid, n, s);  // return to prev spd
}

static void single_switch_throw(uint16_t tid, uint16_t n, enum SWITCH_DIR d) {
    // 2 cmds ==> switch(d, n) & then turn off solenoid
    char cmds[CMD_BYTES + 1] = {(char)d, n, SOL_OFF};
    Putl(tid, cmds, CMD_BYTES + 1);
}

// for switching, send off signal
void switch_throw(uint16_t tid, uint16_t n, enum SWITCH_DIR d) {
    single_switch_throw(tid, n, d);

    // if SW1 switch, need to throw partner in opposite dir
    if (SW1_BASE <= n && n < SW1_BASE + N_SW1) { // sw1?
        single_switch_throw(tid, GET_SW1_PAIR(n), (d == STRT) ? CRV : STRT);
    }
}

void track_go(uint16_t tid) {
    Putc(tid, CMD_GO_BYTE);
}

void track_stop(uint16_t tid) {
    Putc(tid, CMD_STP_BYTE);
}

void init_track(uint16_t tid) {
    track_go(tid);

    // stop all trains
    for (uint16_t tr = 0; tr < N_TRNS; ++tr)
        train_mod_speed_unchecked(tid, ALL_TRNS[tr], SP_STOP);

    // switches
    for (uint16_t sw = SW1_BASE; sw < SW1_BASE + N_SW1; ++sw)
        single_switch_throw(tid, sw, GET_START_SW_STATE(sw));

    Putc(tid, S88_RESET); // sensor reset mode
}

void shutdown_track(uint16_t tid) {
    // stop all trains
    for (uint16_t tr = 0; tr < N_TRNS; ++tr)
        train_mod_speed_unchecked(tid, ALL_TRNS[tr], SP_STOP);

    track_stop(tid);

    // wait for all commands to finish sending
    WaitOutputEmpty(tid);
}
