#include "event-queue.h"

// lib
#include "util.h"

// kernel
#include "task-state.h"

void event_queue_init(event_queue *eq) {
    memset(eq->front, 0, N_EVENTS * sizeof(task_t *));
    memset(eq->back, 0, N_EVENTS * sizeof(task_t *));
}

bool event_queue_block(event_queue *eq, task_t *t, int event_id) {
    if (0 <= event_id && event_id < N_EVENTS) { // valid?
        if (eq->back[event_id] == NULL) {   // empty queue?
            eq->front[event_id] = t;
        } else {                            // non-empty
            eq->back[event_id]->event_wait_next = t;
        }

        eq->back[event_id] = t;

        t->ready_state = STATE_EVENT_WAIT; // block
        return true; // success
    }

    return false; // invalid event_id
}

bool event_queue_unblock_one(event_queue *eq, enum EVENT e, int retval) {
    // equivalent to popping
    task_t *head;
    if (!(head = eq->front[e])) return false; // nothing to pop

    eq->front[e] = head->event_wait_next;
    if (!head->event_wait_next) eq->back[e] = NULL; // emptied queue
    else head->event_wait_next = NULL; // no longer on queue

    // populate return code based on event
    head->x0 = retval;

    head->ready_state = STATE_READY; // unblock

    return true; // success
}

bool event_queue_unblock_all(event_queue *eq, enum EVENT e, int retval) {
    if (!eq->front[e]) return false;

    while (event_queue_unblock_one(eq, e, retval)); // spin
    return true;
}
