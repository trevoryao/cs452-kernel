#include "stopwatch.h"

// lib
#include "sys-clock.h"
#include "util.h"

void stopwatch_init(stopwatch *stpw) {
    memset(stpw->entries, 0, N_STPW * sizeof(stopwatch_entry));
}

void stopwatch_start(stopwatch *stpw, enum STPWS n) {
    stpw->entries[n].started = true; // don't care if we already had one started, just discard
    stpw->entries[n].start_tick = get_curr_ticks();
}

void stopwatch_end(stopwatch *stpw, enum STPWS n) {
    uint64_t end_tick = get_curr_ticks();
    if (!stpw->entries[n].started) return; // started a timer?
    stpw->entries[n].started = false; // finish the timer

    stpw->entries[n].total_ticks += (end_tick - stpw->entries[n].start_tick);
}

uint64_t stopwatch_get_total_ticks(stopwatch *stpw, enum STPWS n) {
    return stpw->entries[n].total_ticks;
}

void stopwatch_get_total_time(stopwatch *stpw, enum STPWS n, time_t *t) {
    time_from_ticks(t, stopwatch_get_total_ticks(stpw, n));
}
