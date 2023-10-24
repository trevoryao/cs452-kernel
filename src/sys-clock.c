#include "sys-clock.h"

// Set-up registers for timer things
static char *const TIMER_BASE = (char *)0xFE003000;

// timer register access
#define TIMER_CS (*(volatile uint32_t *)TIMER_BASE)

#define TIMER_REG(offset) (*(volatile uint32_t *)(TIMER_BASE + offset))

static const uint32_t TIMER_CTR_OFFSETS[2] = {0x04, 0x08};
#define TIMER_CTR_REG(reg) (*(volatile uint32_t*)(TIMER_BASE + TIMER_CTR_OFFSETS[reg]))

static const uint32_t TIMER_CMP_OFFSETS[4] = {0x0c, 0x10, 0x14, 0x18};
#define TIMER_CMP_REG(reg) (*(volatile uint32_t*)(TIMER_BASE + TIMER_CMP_OFFSETS[reg]))

uint64_t get_curr_ticks(void) {
    volatile uint32_t hi = TIMER_CTR_REG(1); // read hi first, less likely to change
    volatile uint32_t lo = TIMER_CTR_REG(0);

    return ((uint64_t)hi << 32) | lo; // combine
}

uint32_t get_curr_lo_ticks(void) { return TIMER_CTR_REG(0); }
uint32_t get_curr_hi_ticks(void) { return TIMER_CTR_REG(1); }

void set_timer(uint16_t n, uint32_t ms) {
    if (n > 3) return;

    volatile uint32_t lo = TIMER_CTR_REG(0); // take only LO bits
    TIMER_CMP_REG(n) = (uint32_t)(lo + (ms * SYSTICKS_MSEC_UNIT)); // write offset of cur time + ms
}

void reset_timer(uint16_t n) {
    if (n > 3) return;
    TIMER_CS = ((uint32_t)1u << n); // write 1 to required bit
}
