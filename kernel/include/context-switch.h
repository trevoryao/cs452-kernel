#ifndef __CONTEXT_SWITCH_H__
#define __CONTEXT_SWITCH_H__

#include <task-state.h>

/*
 * code for performing context switch from user task -> kernel task and vice versa.
 * wrapper for context-switch.S
 */

// must be called in init
extern void init_exception_handlers(void);

// saves kernel context and loads context of "curr_task"
extern void context_switch_out(void);

// saves user context to "curr_task" and loads kernel context in
// returns syscall number when switched back
extern void context_switch_in(void);

// called if incorrect exception/interrupt thrown
// kills task
void unimplemented_handler(void *addr, uint64_t esr);

// displays the current exception level. Mostly used for testing.
void print_el(void);

#endif
