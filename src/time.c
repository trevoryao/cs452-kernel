#include "time.h"

#include "sys-clock.h"

static const uint64_t SYSTICKS_TSEC_UNIT = SYSTICKS_MSEC_UNIT * 100; // 10th of a second
static const uint64_t SYSTICKS_SEC_UNIT = SYSTICKS_TSEC_UNIT * 10;
static const uint64_t SYSTICKS_MIN_UNIT = SYSTICKS_SEC_UNIT * 60;

static const uint32_t CLOCKTICK_TSEC_UNIT = 10;
static const uint32_t CLOCKTICK_SEC_UNIT = CLOCKTICK_TSEC_UNIT * 10;
static const uint32_t CLOCKTICK_MIN_UNIT = CLOCKTICK_SEC_UNIT * 60;


void time_from_sys_ticks(time_t *t, uint64_t ticks) {
    t->min = ticks / SYSTICKS_MIN_UNIT;

    uint64_t sec_ctr = ticks % SYSTICKS_MIN_UNIT;
    t->sec = sec_ctr / SYSTICKS_SEC_UNIT;

    uint64_t tsec_ctr = sec_ctr % SYSTICKS_SEC_UNIT;
    t->tsec = tsec_ctr / SYSTICKS_TSEC_UNIT;
}

void time_from_clock_ticks(time_t *t, uint32_t ticks) {
    t->min = ticks / CLOCKTICK_MIN_UNIT;

    uint64_t sec_ctr = ticks % CLOCKTICK_MIN_UNIT;
    t->sec = sec_ctr / CLOCKTICK_SEC_UNIT;

    uint64_t tsec_ctr = sec_ctr % CLOCKTICK_SEC_UNIT;
    t->tsec = tsec_ctr / CLOCKTICK_TSEC_UNIT;
}

