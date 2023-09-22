#include "task-queue.h"

// local definition of methods
task_t *task_queue_pop(task_queue *tq, enum PRIORITY p); // pops highest priority present
void task_queue_push(task_queue *tq, task_t *task);

bool task_queue_priority_is_empty(task_queue *tq, enum PRIORITY p);
uint32_t task_queue_priority_size(task_queue *tq, enum PRIORITY p);

uint16_t task_queue_get_tid(task_queue *tq);

void task_queue_init(task_queue *tq) {
    // initialize the queue pointers
    // NULL usually defined as 0
    memset(tq->front, 0, N_PRIORITY);
    memset(tq->back, 0, N_PRIORITY);

    // on init only kernel running
    tq->highest_tid = 0;
}

task_t *task_queue_schedule(task_queue *tq) {    
    // For Now: highest priority always first -> if not go lower
    for (enum PRIORITY p = P_VHIGH; p >= P_VLOW; p--) {
        // if empty continue
        if (task_queue_priority_is_empty(tq, p)) {
            continue;
        }
        
        task_t *current = NULL;
        task_t *old_back = tq->back[p];

        // go through the entire loop
        while(current != old_back) {
            // get next element in queue
            current = task_queue_pop(tq, p);

            if (current->ready_state == STATE_READY) {
                return current;
            } else {
                task_queue_push(tq, current);
            }
        }
    } 
    return NULL;
}

void task_queue_add(task_queue *tq, task_t *task) {
    // assign the new task a tid, if it is zero
    if (task->tid == 0) {
        uint16_t tid = task_queue_get_tid(tq);
        task->tid = tid;
    }
    task_queue_push(tq, task);
}

bool task_queue_priority_is_empty(task_queue *tq, enum PRIORITY p) {
    return (tq->front[p] == NULL && tq->back[p] == NULL);
}

uint32_t task_queue_priority_size(task_queue *tq, enum PRIORITY p) {
    uint32_t size = 0;
    task_t *current = tq->front[p];

    while (current != NULL) {
        size += 1;
        current = current->next;
    }
    return size;
}

task_t *task_queue_pop(task_queue *tq, enum PRIORITY p) {
    task_t *head = tq->front[p];
    if (head != NULL) {
        tq->front[p] = head->next;
    }
    if(head->next == NULL) {
        tq->back[p] = NULL;
    }

    // set head next to NULL
    head->next = NULL;

    return head;
}

void task_queue_push(task_queue *tq, task_t *task) {
    enum PRIORITY p = task->priority;
    // make sure that task next is NULL
    task->next = NULL;
    
    if(tq->back[p] == NULL) {
        // Case Queue is empty
        tq->front[p] = task;
        tq->back[p] = task;
    } else {
        // Case Queue not empty
        tq->back[p]->next = task;
        tq->back[p] = task;
    }
}

uint16_t task_queue_get_tid(task_queue *tq) {
    tq->highest_tid += 1;
    return tq->highest_tid;
}

void task_queue_free_tid(task_queue *tq, uint32_t tid) {
    // only need to do anything if highest tid is cleared
    if (tid == tq->highest_tid) {
        // go through all queues finding the next highest tid
        uint16_t new_highest = 0;
        
        for (enum PRIORITY priority = P_VHIGH; priority > P_VLOW; priority--) {
        // go through list -> check if available -> if not push it back to the end of queue.
            task_t *current_element = tq->front[priority];
            while(current_element) {
                if (current_element->tid > new_highest) {
                    new_highest = current_element->tid;
                }
                current_element = current_element->next;
            }
        } 
        tq->highest_tid = new_highest;
    }
}
