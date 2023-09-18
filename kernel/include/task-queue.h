#ifndef __TASK_QUEUE_H__
#define __TASK_QUEUE_H__

#include <task-state.h>
#include <stdbool.h>
#include <stdint.h>

/*
 * data structure for managing queued tasks.
 * implements an intrusive linked-list priority queue.
 */

typedef struct task_queue {
    // array of FIFOs

    task_t *front[N_PRIORITY];
    task_t *back[N_PRIORITY];
} task_queue;

void task_queue_init(task_queue *tq);

task_t *task_queue_pop(task_queue *tq); // pops highest priority present
void task_queue_push(task_queue *tq, task_t *task);

bool task_queue_is_empty(task_queue *tq);
bool task_queue_is_full(task_queue *tq);
uint32_t task_queue_size(task_queue *tq);

bool task_queue_priority_is_empty(task_queue *tq, enum PRIORITY p);
bool task_queue_priority_is_full(task_queue *tq, enum PRIORITY p);
uint32_t task_queue_priority_size(task_queue *tq, enum PRIORITY p);

#endif
