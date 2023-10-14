#ifndef __STOPWATCH_H__
#define __STOPWATCH_H__

#include <stdbool.h>
#include <stdint.h>

typedef struct time_t time_t;

// functions used for a general stopwatch

typedef struct stopwatch_entry {
    uint64_t total_ticks;
    uint64_t start_tick;

    bool started; // flag
} stopwatch_entry;

enum STPWS {
    STPW_IDLE_TASK,
    N_STPW
};

typedef struct stopwatch {
    stopwatch_entry entries[N_STPW];
} stopwatch;

void stopwatch_init(stopwatch *stpw);

void stopwatch_start(stopwatch *stpw, enum STPWS n);
void stopwatch_end(stopwatch *stpw, enum STPWS n);

uint64_t stopwatch_get_total_ticks(stopwatch *stpw, enum STPWS n);
void stopwatch_get_total_time(stopwatch *stpw, enum STPWS n, time_t *t);


#endif
