#ifndef __TASK_ALLOC_H__
#define __TASK_ALLOC_H__

#include <stddef.h>
#include <string.h>

// kernel
#include "task-state.h"

/*
 * provides functions for managing the memory of tasks
 * (slab allocation)
 */

// 8 tasks per slab
#define N_TASK_T 8
#define N_SLABS 128

typedef struct task_alloc {
    task_t slabs[N_SLABS][N_TASK_T];

    // free list
    task_t *free_list;

    // Bitfield representing active slabs
    uint8_t active_slabs[N_SLABS];

    // array representing how many elements are in each slab
    uint8_t task_count_in_slab[N_SLABS];
} task_alloc;

void task_alloc_init(task_alloc *alloc);
task_t *task_alloc_new(task_alloc *alloc);
void task_alloc_free(task_alloc *alloc, task_t *task);
void task_alloc_add_to_free_list(task_alloc *alloc, task_t *task);

#endif
