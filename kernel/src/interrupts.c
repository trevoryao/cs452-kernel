#include "interrupts.h"

// kernel
#include "event-queue.h"
#include "kassert.h"
#include "task-state.h"

// lib
#include "sys-clock.h"

// set-up GIC registers
static char *const GIC_BASE     = (char *)0xFF840000;
static char *const GICD_BASE    = (char *)(GIC_BASE + 0x1000);
static char *const GICC_BASE    = (char *)(GIC_BASE + 0x2000);

static char *const GICD_ITARGETS_BASE = (char *)(GICD_BASE + 0x800);
static char *const GICD_ISENABLE_BASE = (char *)(GICD_BASE + 0x100);

#define GICD_ITARGETS_Rn(m) (*(volatile uint32_t *)(GICD_ITARGETS_BASE + ((m) << 2))) // mult 4
#define GICD_ISENABLE_Rn(m) (*(volatile uint32_t *)(GICD_ISENABLE_BASE + ((m) << 2))) // mult 4

static const uint32_t IAR = 0x0c;
static const uint32_t EOIR = 0x10;

#define GICC_REG(offset) (*(volatile uint32_t *)(GICC_BASE + offset))

// const
#define TIMER_TICK_REG 1

// interrupt IDs
#define TIMER_C1_INTERRUPT_ID 97
#define TIMER_C3_INTERRUPT_ID 99

static const uint16_t N_INTERRUPT_IDS = 2;
static const uint32_t INTERRUPT_IDS[] = { TIMER_C1_INTERRUPT_ID, TIMER_C3_INTERRUPT_ID };

static const uint32_t CPU0_MSK = 0x01;

// map enum EVENT to GICD interrupt IDs (and vice versa)
static const uint32_t interrupt_ids_from_event[N_EVENTS] = { TIMER_C1_INTERRUPT_ID };

static enum EVENT event_from_interrupt_id(uint32_t interrupt_id) {
    // boo, ugly, look away (but fast)
    switch (interrupt_id) {
        case TIMER_C1_INTERRUPT_ID: return TIMER_TICK;
        default: kpanic("received unknown interrupt id %u\r\n", interrupt_id);
    }

    return N_EVENTS; // never gets here
}

void init_interrupt_handlers(void) {
    for (uint16_t i = 0; i < N_INTERRUPT_IDS; ++i) {
        // enabling
        uint32_t enable_n = INTERRUPT_IDS[i] >> 5; // div 32
        uint32_t enable_offset = INTERRUPT_IDS[i] & 31; // modulo 32
        GICD_ISENABLE_Rn(enable_n) = (uint32_t)1u << enable_offset;

        // targetting
        uint32_t target_n = INTERRUPT_IDS[i] >> 2; // div 4
        uint32_t target_offset = (INTERRUPT_IDS[i] & 3) << 3; // modulo 4; mult 8
        GICD_ITARGETS_Rn(target_n) |= (CPU0_MSK << target_offset);
    }
}

void trigger_interrupt(enum EVENT e) {
    KLOG("interrupt %d triggered\r\n", e);

        switch (e) {
            case TIMER_TICK: {
                set_timer(TIMER_TICK_REG, TICK_MS);
                break;
            }
        default: kpanic("unexpected interrupt event %d requested\r\n", e);
    }
}

void handle_interrupt(event_queue *eq) {
    uint32_t interrupt_id = GICC_REG(IAR);

    KLOG("handling interrupt %d\r\n", interrupt_id);

    switch (event_from_interrupt_id(interrupt_id)) {
        case TIMER_TICK: {
            reset_timer(TIMER_TICK_REG);
            event_queue_unblock_all(eq, TIMER_TICK);
            break;
        }
        default: kpanic("unexpected interrupt occurred (id %u)\r\n", interrupt_id);
    }

    GICC_REG(EOIR) = interrupt_id; // finish handling interrupt
}