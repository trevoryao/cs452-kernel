#ifndef __TASK_QUEUE_H__
#define __TASK_QUEUE_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// kernel
#include "task-state.h"

// lib
#include "util.h"

/*
 * data structure for managing queued tasks.
 * implements an intrusive linked-list priority queue.
 */

typedef struct task_queue {
    // array of FIFOs
    task_t *front[N_PRIORITY];
    task_t *back[N_PRIORITY];

    // assigning Ids missing here
    uint16_t global_tid;

} task_queue;

void task_queue_init(task_queue *tq);
task_t *task_queue_schedule(task_queue *tq);
int32_t task_queue_add(task_queue *tq, task_t *task);
bool task_queue_empty(task_queue *tq);
void task_queue_free_tid(task_queue *tq, uint16_t tid);

#endif
