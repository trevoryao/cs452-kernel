#ifndef __CONTEXT_SWITCH_H__
#define __CONTEXT_SWITCH_H__

#include <stdint.h>

typedef struct task_t task_t;
typedef struct kernel_state kernel_state;

/*
 * code for performing context switch from user task -> kernel task and vice versa.
 * wrapper for context-switch.S
 */

// must be called in init
extern void init_exception_handlers(void);

// saves kernel context and loads context of "curr_task"
extern void context_switch_out(task_t *curr_user_task, kernel_state *kernel_task);

// saves user context to "curr_task" and loads kernel context in
// returns syscall number when switched back
extern void context_switch_in(void);

// called if incorrect exception/interrupt thrown
// kills task
void unimplemented_handler(void *addr, uint64_t esr, uint64_t pc);

// displays the current exception level. Mostly used for testing.
void print_el(void);

#endif
