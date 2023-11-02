#ifndef  __PRIORITY_QUEUE_H__
#define __PRIORITY_QUEUE_H__

#include <stdbool.h>
#include <stdint.h>

#define MAX_PQ_ELEMENTS 256
#define PQ_EMPTY INT16_MAX + 1

/*
 * generic (int, int) min-priority queue implemented as a binary heap
 */

typedef struct heap_node {
    int16_t priority;
    int16_t value;
} heap_node;

typedef struct priority_queue {
    heap_node heap[MAX_PQ_ELEMENTS];
    int size;
} priority_queue;

// assumes heap array already has all elements added
void priority_queue_init(priority_queue *pq);

void priority_queue_add(priority_queue *pq, int16_t val, int16_t priority);

int32_t priority_queue_pop_min(priority_queue *pq);

// returns true on success
bool priority_queue_decrease(priority_queue *pq, int16_t val, int16_t new_priority);

#endif
