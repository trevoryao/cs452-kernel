#ifndef __TASK_STATE_H__
#define __TASK_STATE_H__

#include <stdint.h>

/*
 * data structure for representing task state
 */

#define STACK_SIZE 400000 // 400 kB

enum READY_STATE {
    N_READY_STATE
};

enum PRIORITY {
    N_PRIORITY
};

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

    int64_t pc;
    int64_t sp;

    int64_t pstate;

    // state ends

    uint16_t tid;
    task_t *parent;

    enum READY_STATE ready_state;
    enum PRIORITY priority;

    char stack[STACK_SIZE];

    task_t *next; // intrusive linked list
} task_t;

#endif
