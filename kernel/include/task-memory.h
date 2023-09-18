#ifndef __TASK_MEMORY_H__
#define __TASK_MEMORY_H__

#include <task-state.h>

/*
 * provides functions for managing the memory of tasks
 * (slab allocation)
 */

#define N_TASK_T 1024

typedef struct task_alloc {
    // fill me
    // array of slabs

    // free list
} task_alloc;

void task_alloc_init(task_alloc *alloc);

task_t *task_alloc_new(void);
void task_alloc_free(task_t *task);

#endif
