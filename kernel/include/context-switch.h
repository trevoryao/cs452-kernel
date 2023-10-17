#ifndef __CONTEXT_SWITCH_H__
#define __CONTEXT_SWITCH_H__

// bit masks used when returning from context switch
#define CURR_SPO_MSK    0x10
#define CURR_SPn_MSK    0x20
#define LOWER_EL_64_MSK 0x40
#define LOWER_EL_32_MSK 0x80

#define SYNC_MSK        0x01
#define IRQ_MSK         0x02
#define FIQ_MSK         0x04
#define SERROR_MSK      0x08

#ifndef __ASSEMBLER__ // exclude from asm file

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
// returns the handler which handled it as a single 8 byte value as follows:
// bytes [0:3] indicate which exception type (sync/irq/fiq/serror)
// bytes [4:7] indicate the description (current/lower EL, SP, 64/32bits)
// both are ordered as in the EL0 table
// see top of file for the bit masks
extern uint8_t context_switch_out(task_t *curr_user_task, kernel_state *kernel_task);


// called if incorrect exception/interrupt thrown
// kills task
void unimplemented_handler(void *addr, uint64_t esr, uint64_t pc);

// displays the current exception level. Mostly used for testing.
void print_el(void);

#endif // __ASSEMBLER__
#endif // __CONTEXT_SWITCH_H__
