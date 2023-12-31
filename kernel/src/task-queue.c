#include "task-queue.h"

// kernel
#include "kassert.h"

// lib
#include "util.h"

#define WRAP_AROUND_TID 300 // unlikely to have long running processes this high

static bool task_queue_priority_is_empty(task_queue *tq, enum PRIORITY p) {
    return (tq->front[p] == NULL && tq->back[p] == NULL);
}

static void task_queue_push(task_queue *tq, task_t *task) {
    enum PRIORITY p = task->priority;
    // make sure that task next is NULL
    task->next = NULL;

    if (tq->back[p] == NULL) {
        // Case Queue is empty
        tq->front[p] = task;
        tq->back[p] = task;
    } else {
        // Case Queue not empty
        tq->back[p]->next = task;
        tq->back[p] = task;
    }
}

static int32_t task_queue_get_tid(task_queue *tq) {
    // follows UNIX algorithm for assigning TIDs
    // wrap around to WRAP_AROUND_TID and check if its free
    // if not, continue checking
    // higher WRAP_AROUND_TID decreases odds of a lot of iterations

    int32_t tid = (tq->global_tid == UINT16_MAX) ? WRAP_AROUND_TID : (tq->global_tid + 1);

    while (tid <= UINT16_MAX) {
        task_t *dup = task_queue_get(tq, tid);

        if (!dup) {
            // finished iteration without finding duplicate tid
            tq->global_tid = (uint16_t)tid;
            return tq->global_tid;
        } else {
            ++tid;
        }
    }

    // somehow iterated all tasks without finding a free priority
    KLOG("no TID\r\n");
    return -1; // out of TIDs
}

void task_queue_init(task_queue *tq) {
    // initialize the queue pointers
    // NULL usually defined as 0
    memset(tq->front, 0, N_PRIORITY * sizeof(task_t *));
    memset(tq->back, 0, N_PRIORITY * sizeof(task_t *));

    // on init only kernel running
    tq->global_tid = 50; // account for long running kernel tasks which start
}

task_t *task_queue_schedule(task_queue *tq) {
    // For Now: highest priority always first -> if not go lower
    for (int p = N_PRIORITY - 1; p >= 0; --p) {
        // if empty continue
        if (task_queue_priority_is_empty(tq, p)) {
            continue;
        }

        // do we want to just have the first ready task skip the line?
        // otherwise when the task is eventually unblocked we want it to
        // resume running immediately

        task_t *current = tq->front[p];
        task_t *last = NULL;

        // go through the entire loop
        while (current != NULL) {
            // get next element in queue
            if (current->ready_state == STATE_READY ||
                current->ready_state == STATE_KILLED) {
                // Case 1: current is the front of the queue
                if (current == tq->front[p]) {
                    tq->front[p] = current->next;
                } else {
                    // Case 2: current is not the front of the queue
                    last->next = current->next;
                }

                //  set the back of the queue if necessary
                if (current == tq->back[p]) {
                    tq->back[p] = last;
                }

                // NULL the next pointer of the current
                current->next = NULL;
                return current;
            } else {
                last = current;
                current = current->next;
            }
        }
    }

    KLOG("No avail task\r\n");
    return NULL;
}

int32_t task_queue_add(task_queue *tq, task_t *task) {
    kassert(task);

    // assign the new task a tid, if it is zero
    if (task->ready_state == STATE_RUNNING)
        task->ready_state = STATE_READY;

    if (task->tid == 0) {
        int16_t tid = task_queue_get_tid(tq);
        if (tid < 0) {
            KLOG("Dropping task-%d\r\n", task->tid);
            return tid; // out of tids
        }
        task->tid = tid;
    }

    task_queue_push(tq, task);

    return task->tid;
}

bool task_queue_empty(task_queue *tq) {
    // ignore P_IDLE & server priorities
    for (int p = P_IDLE + 1; p < N_PRIORITY - N_SERVER_PRIORITY; ++p) {
        if (!task_queue_priority_is_empty(tq, p)) return false;
    }

    return true;
}

void task_queue_free_tid(task_queue *tq, uint16_t tid) {
    for (int priority = N_PRIORITY - 1; priority >= 0; --priority) {
        task_t *current_element = tq->front[priority];
        while (current_element) {
            // clear child nodes of parent
            if (current_element->parent->tid == tid) {
                current_element->parent = NULL;
            }
            current_element = current_element->next;
        }
    }
}

task_t *task_queue_get(task_queue *tq, uint16_t tid) {
    for (int p = N_PRIORITY - 1; p >= 0; --p) {
        task_t *cur = tq->front[p];

        while (cur) {
            if (cur->tid == tid) return cur;
            cur = cur->next; // otherwise keep checking
        }
    }

    return NULL;
}

void task_queue_kill_children(task_queue *tq, uint16_t ptid) {
    // ignore P_IDLE & server priorities
    for (int p = P_IDLE + 1; p < N_PRIORITY - N_SERVER_PRIORITY; ++p) {
        task_t *cur = tq->front[p];
        while (cur) {
            if (cur->parent->tid == ptid) {
                cur->ready_state = STATE_KILLED; // lazy kill, let scheduler determine if killed
                task_queue_kill_children(tq, cur->tid);
            }
            cur = cur->next;
        }
    }
}
