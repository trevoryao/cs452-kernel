#ifndef __TRACK_H__
#define __TRACK_H__

#include <stdint.h>

#include "k4/controller-consts.h"

/*
 * helper f'ns for sending bytes to the controller
 * all f'ns take tid of marklin output server and
 * add/remove necessary bytes to the output deque
 */

typedef struct speed_t speed_t;

// setup/teardown f'ns
void init_track(uint16_t tid);
void shutdown_track(uint16_t tid);

void train_mod_speed(uint16_t tid, speed_t *spd_t, uint16_t n, uint16_t s);

void train_reverse_start(uint16_t tid, speed_t *spd_t, uint16_t n);
void train_reverse_end(uint16_t tid, uint16_t s, uint16_t n);

void switch_throw(uint16_t tid, uint16_t n, enum SWITCH_DIR d);

void track_go(uint16_t tid);
void track_stop(uint16_t tid);

#endif
