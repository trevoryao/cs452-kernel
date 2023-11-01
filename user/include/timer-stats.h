#ifndef __TIMER_STATS_H__
#define __TIMER_STATS_H__

#include <stdint.h>

/*
 * struct for timing stats on execution
 */

// timers
enum TMR_STATS {
    PERF_TMR,
    N_TMRS
};

// track cumulative avg
typedef struct stat_t {
    uint64_t avg; // cumulative counter
    uint64_t n; // number in avg

    uint64_t start_tick; // mark the start of the current timer

    uint64_t max; // max counter
    uint64_t min;
} stat_t;

typedef struct timer_stats {
    stat_t stats[N_TMRS];
} timer_stats;

void timer_stats_init(timer_stats *s);

void timer_stats_start(timer_stats *s, enum TMR_STATS t);
uint64_t timer_stats_end(timer_stats *s, enum TMR_STATS t);

void timer_stats_clear(timer_stats *s, enum TMR_STATS t);

void timer_stats_print_avg(timer_stats *s, enum TMR_STATS t);
void timer_stats_print_max(timer_stats *s, enum TMR_STATS t);

#endif
