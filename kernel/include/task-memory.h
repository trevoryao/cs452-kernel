#ifndef __TASK_MEMORY_H__
#define __TASK_MEMORY_H__

#include <task-state.h>
#include <stddef.h>

/*
 * provides functions for managing the memory of tasks
 * (slab allocation)
 */

// 8 tasks per slab
#define N_TASK_T 8
#define N_SLABS 128


typedef struct task_alloc {
    // fill me
    // array of slabs
    
    // TODO: Q: Is that how you wanted to use the constant? -> N of tasks would then be 8*N_TASK_T
    task_t slabs[N_SLABS][N_TASK_T];

    // free list
    task_t *free_list;

    // Bitfield representing active slabs
    uint8_t active_slab;

    // array representing how many elements are in each slab
    uint8_t task_count_in_slab[8];
} task_alloc;

void task_alloc_init(task_alloc *alloc);
task_t *task_alloc_new(task_alloc *alloc);
void task_alloc_free(task_alloc *alloc, task_t *task);

#endif
