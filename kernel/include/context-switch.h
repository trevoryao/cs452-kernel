#ifndef __CONTEXT_SWITCH_H__
#define __CONTEXT_SWITCH_H__

#include <task-state.h>

/*
 * code for performing context switch from user task -> kernel task and vice versa.
 * wrapper for context-switch.S
 */

// saves current context into `cur` and loads context of `next` into program registers
extern void context_switch(task_t *cur, task_t *next);

#endif
