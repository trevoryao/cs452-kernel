#ifndef __SYS_CLOCK_H__
#define __SYS_CLOCK_H__

#include <stdint.h>

// time units defined in terms of timer_ctr increments
static const uint64_t TIMER_MSEC_UNIT = 1000; // 1mhz
static const uint64_t TIMER_TICK_UNIT = TIMER_MSEC_UNIT * 10;
static const uint64_t TIMER_TSEC_UNIT = TIMER_MSEC_UNIT * 100; // 10th of a second
static const uint64_t TIMER_SEC_UNIT = TIMER_TSEC_UNIT * 10;
static const uint64_t TIMER_MIN_UNIT = TIMER_SEC_UNIT * 60;

#define TICK_MS 10 // 10ms for a single clock tick

/*
 * interface for interacting with the timer peripheral registers
 */

// formatting structure for raw time ticks
typedef struct time_t {
    uint64_t min, sec, tsec; // for the system timer
} time_t;

// raw ticks
uint32_t get_curr_lo_ticks(void);
uint32_t get_curr_hi_ticks(void);

uint64_t get_curr_ticks(void);
void time_from_ticks(time_t *t, uint64_t ticks);

// set_timer Cn for ms milliseconds
void set_timer(uint16_t n, uint32_t ms);
void reset_timer(uint16_t n);

#endif
