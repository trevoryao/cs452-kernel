#ifndef __SYS_CLOCK_H__
#define __SYS_CLOCK_H__

#include <stdint.h>

/*
 * interface for interacting with the timer peripheral registers
 */

// formatting structure for raw time ticks
struct time_t {
    uint64_t min, sec, tsec; // for the system timer
};

// raw ticks
uint32_t get_curr_lo_ticks(void);
uint32_t get_curr_hi_ticks(void);

uint64_t get_curr_ticks(void);
void time_from_ticks(struct time_t *t, uint64_t ticks);

// set_timer Cn for ms milliseconds
void set_timer(uint16_t n, uint32_t ms);
int check_timer(uint16_t n); // clears timer if match occurred

// returns 1 if at least ms has passed between start and current time
int check_tick_passed(uint64_t start, uint32_t ms);

#endif
