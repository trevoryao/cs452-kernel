#ifndef __TASK_STATE_H__
#define __TASK_STATE_H__

#include <stdint.h>

/*
 * data structure for representing task state
 */

#define STACK_SIZE 400000 // 400 kB

enum READY_STATE {
    STATE_READY,
    STATE_BLOCKED,
    STATE_KILLED,
    N_READY_STATE
};

enum PRIORITY {
    P_VHIGH,
    P_HIGH,
    P_MED,
    P_LOW,
    P_VLOW,
    N_PRIORITY
};

// Forward definition. Otherwise you cannot refer in task_t to itself -> compiler error
typedef struct task_t task_t;

typedef struct task_t {
    // hardcoded assembly -- do not reorder or add before
    // state until after state ends
    // state begins
    int64_t x0;
    int64_t x1;
    int64_t x2;
    int64_t x3;
    int64_t x4;
    int64_t x5;
    int64_t x6;
    int64_t x7;
    int64_t x8;
    int64_t x9;
    int64_t x10;
    int64_t x11;
    int64_t x12;
    int64_t x13;
    int64_t x14;
    int64_t x15;
    int64_t x16;
    int64_t x17;
    int64_t x18;
    int64_t x19;
    int64_t x20;
    int64_t x21;
    int64_t x22;
    int64_t x23;
    int64_t x24;
    int64_t x25;
    int64_t x26;
    int64_t x27;
    int64_t x28;
    int64_t x29;
    int64_t x30;

    uint64_t pc;
    uint64_t sp;

    int64_t pstate;

    // state ends

    uint16_t tid;
    task_t *parent; // if no parent, created by kernel

    enum READY_STATE ready_state;
    enum PRIORITY priority;

    char stack[STACK_SIZE];

    task_t *next; // intrusive linked list
    uint8_t slab_index;
} task_t;

void task_init(task_t *t, void (*function)(), task_t *parent);

typedef struct kernel_state {
    // hardcoded assembly -- do not reorder or add before
    // kernel state for context switching
    int64_t x0;
    int64_t x1;
    int64_t x2;
    int64_t x3;
    int64_t x4;
    int64_t x5;
    int64_t x6;
    int64_t x7;
    int64_t x8;
    int64_t x9;
    int64_t x10;
    int64_t x11;
    int64_t x12;
    int64_t x13;
    int64_t x14;
    int64_t x15;
    int64_t x16;
    int64_t x17;
    int64_t x18;
    int64_t x19;
    int64_t x20;
    int64_t x21;
    int64_t x22;
    int64_t x23;
    int64_t x24;
    int64_t x25;
    int64_t x26;
    int64_t x27;
    int64_t x28;
    int64_t x29;
    int64_t x30;

    uint64_t pc;
    uint64_t sp;

    int64_t pstate;
} kernel_state;

#endif
