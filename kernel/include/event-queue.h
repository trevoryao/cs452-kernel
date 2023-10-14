#ifndef __EVENT_QUEUE_H__
#define __EVENT_QUEUE_H__

// lib
#include "events.h"

#include <stdbool.h>

typedef struct task_t task_t;

/*
 * Queue for blocked tasks awaiting an event.
 */

typedef struct event_queue {
    task_t *front[N_EVENTS];
    task_t *back[N_EVENTS];
} event_queue;

void event_queue_init(event_queue *eq);

// returns true on success (only returns false on invalid event_id)
bool event_queue_block(event_queue *eq, task_t *t, int event_id);

// returns false if nothing to unblock, true otherwise
bool event_queue_unblock_one(event_queue *eq, enum EVENT e);
bool event_queue_unblock_all(event_queue *eq, enum EVENT e);

#endif
