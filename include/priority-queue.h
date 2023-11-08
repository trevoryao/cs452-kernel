#ifndef  __PRIORITY_QUEUE_H__
#define __PRIORITY_QUEUE_H__

#include <stdbool.h>
#include <stdint.h>

#define MAX_PQ_ELEMENTS 256

/*
 * generic (int, int) min-priority queue implemented as a binary heap
 */

typedef struct heap_node {
    uint16_t priority;
    void *value;
} heap_node;

typedef struct priority_queue {
    heap_node heap[MAX_PQ_ELEMENTS];
    uint16_t size;
} priority_queue;

// assumes heap array already has all elements added
void priority_queue_init(priority_queue *pq, uint16_t size);

void priority_queue_add(priority_queue *pq, void *val, uint16_t priority);

void *priority_queue_pop_min(priority_queue *pq);

// returns true on success
bool priority_queue_decrease(priority_queue *pq, void *val, uint16_t new_priority);

bool priority_queue_empty(priority_queue *pq);

#endif
