#include "timer-stats.h"

// lib
#include "rpi.h"
#include "util.h"

// Set-up registers for timer things
static char *const TIMER_BASE = (char *)0xFE003000;

static const uint32_t TIMER_CTR_OFFSETS[2] = {0x04, 0x08};
#define TIMER_CTR_REG(reg) (*(volatile uint32_t*)(TIMER_BASE + TIMER_CTR_OFFSETS[reg]))

// time units defined in terms of timer_ctr increments
static const uint64_t TIMER_MSEC = 1000; // 1mhz

static uint64_t get_curr_ticks(void) {
    volatile uint32_t hi = TIMER_CTR_REG(1); // read hi first, less likely to change
    volatile uint32_t lo = TIMER_CTR_REG(0);

    return ((uint64_t)hi << 32) | lo; // combine
}

void timer_stats_init(timer_stats *s) {
    memset(s, 0, N_TMRS * sizeof(stat_t)); // all values to 0 (in stat_t)
}

void timer_stats_start(timer_stats *s, enum TMR_STATS t) {
    s->stats[t].started = 1; // don't care if we already had one started, just discard
    s->stats[t].start_tick = get_curr_ticks();
}

void timer_stats_end(timer_stats *s, enum TMR_STATS t) {
    uint64_t end_tick = get_curr_ticks();

    if (!s->stats[t].started) return; // started a timer?

    uint64_t elapsed = end_tick - s->stats[t].start_tick;

    // accumulative avg
    s->stats[t].avg = ((s->stats[t].n * s->stats[t].avg) + elapsed) / (s->stats[t].n + 1);
    ++s->stats[t].n;

    if (elapsed > s->stats[t].max) s->stats[t].max = elapsed;
}

void timer_stats_clear(timer_stats *s, enum TMR_STATS t) {
    memset(s + t, 0, sizeof(timer_stats));
}

void timer_stats_print_avg(timer_stats *s, enum TMR_STATS t) {
    uart_printf(CONSOLE, "Avg: %u microseconds\r\n", s->stats[t].avg);
}

void timer_stats_print_max(timer_stats *s, enum TMR_STATS t) {
    uart_printf(CONSOLE, "Max: %u microseconds\r\n", s->stats[t].max);
}
