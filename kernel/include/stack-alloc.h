#ifndef __STACK_ALLOC_H__
#define __STACK_ALLOC_H__

#include <stdint.h>

// kernel
#include "task-alloc.h"

/*
 * Allocator for task stacks.
 */

#define STACK_SIZE 0x80000 // ~500 kB

typedef struct stack_alloc {
    int8_t is_allocd[N_TASK_T]; // tracker of alloc'd tasks
    void *base;
} stack_alloc;

void stack_alloc_init(stack_alloc *salloc, void *kernel_stack_ptr);

// returns ptr to base of the given stack
void *stack_alloc_new(stack_alloc *salloc);

// stack_base should be the same as the one given in the new function
void stack_alloc_free(stack_alloc *salloc, void *stack_base);

#endif
