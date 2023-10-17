#ifndef __CLOCK_QUEUE_H__
#define __CLOCK_QUEUE_H__

#include <stdbool.h>
#include <stdint.h>
#include "util.h"

#define MAX_WAITING_TASKS 256

/*
 * data structure for managing waiting processes.
 * implements an intrusive linked-list priority queue.
 */

typedef struct clock_queue_entry clock_queue_entry;

typedef struct clock_queue_entry {
    // assigning Ids missing here
    uint16_t tid;
    uint64_t clock_tick;
    clock_queue_entry *next; 

} clock_queue_entry;

typedef struct clock_queue {
    clock_queue_entry storage[256];
    clock_queue_entry *freelist;

    clock_queue_entry *head;
} clock_queue;


void clock_queue_init(clock_queue *cq);

// returns if any task can be returned
bool clock_queue_ready(clock_queue *cq, uint64_t curr_clock_tick);

// returns first element that is ready
uint16_t clock_queue_get(clock_queue *cq, uint64_t curr_clock_tick);

// add a new entry sorted to the list
void clock_queue_add(clock_queue *cq, uint16_t tid, uint64_t clock_tick);

#endif
