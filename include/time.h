#ifndef __TIME_H__
#define __TIME_H__

#include <stdint.h>

// formatting structure for raw time ticks
typedef struct time_t {
    uint64_t min, sec, tsec; // for the system timer
} time_t;

void time_from_sys_ticks(time_t *t, uint64_t ticks);
void time_from_clock_ticks(time_t *t, uint32_t ticks);

#endif
