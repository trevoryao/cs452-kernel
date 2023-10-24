#ifndef __SYS_CLOCK_H__
#define __SYS_CLOCK_H__

#include <stdint.h>

// time units defined in terms of timer_ctr increments
static const uint64_t SYSTICKS_MSEC_UNIT = 1000; // 1mhz
static const uint64_t SYSTICKS_CLOCKTICK_UNIT = SYSTICKS_MSEC_UNIT * 10;

#define TICK_MS 10 // 10ms for a single clock tick

/*
 * interface for interacting with the timer peripheral registers
 */

// raw ticks
uint32_t get_curr_lo_ticks(void);
uint32_t get_curr_hi_ticks(void);

uint64_t get_curr_ticks(void);

// set_timer Cn for ms milliseconds
void set_timer(uint16_t n, uint32_t ms);
void reset_timer(uint16_t n);

#endif
