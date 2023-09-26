#include "task-queue.h"

// kernel
#include "kassert.h"

// local definition of methods
static task_t *task_queue_pop(task_queue *tq, enum PRIORITY p); // pops highest priority present
static void task_queue_push(task_queue *tq, task_t *task);

static bool task_queue_priority_is_empty(task_queue *tq, enum PRIORITY p);
static uint32_t task_queue_priority_size(task_queue *tq, enum PRIORITY p);

static int32_t task_queue_get_tid(task_queue *tq);

void task_queue_init(task_queue *tq) {
    // initialize the queue pointers
    // NULL usually defined as 0
    memset(tq->front, 0, N_PRIORITY * sizeof(task_t *));
    memset(tq->back, 0, N_PRIORITY * sizeof(task_t *));

    // on init only kernel running
    tq->highest_tid = 0;
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

        task_t *current = NULL;
        task_t *old_back = tq->back[p];

        // go through the entire loop
        while(current != old_back) {
            // get next element in queue
            current = task_queue_pop(tq, p);

            KLOG("schedule: checking tid %d (state=%d)\r\n", current->tid, current->ready_state);

            if (current->ready_state == STATE_READY) {
                current->ready_state = STATE_RUNNING;
                return current;
            } else {
                task_queue_push(tq, current);
            }
        }
    }

    return NULL;
}

int32_t task_queue_add(task_queue *tq, task_t *task) {
    // assign the new task a tid, if it is zero
    if (task->ready_state == STATE_KILLED)
        return -1; // drop killed task
    else if (task->ready_state == STATE_RUNNING)
        task->ready_state = STATE_READY;

    if (task->tid == 0) {
        int16_t tid = task_queue_get_tid(tq);
        if (tid < 0) return tid; // out of tids
        task->tid = tid;
    }

    task_queue_push(tq, task);

    return task->tid;
}

bool task_queue_empty(task_queue *tq) {
    for (int p = 0; p < N_PRIORITY; ++p) {
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

static bool task_queue_priority_is_empty(task_queue *tq, enum PRIORITY p) {
    return (tq->front[p] == NULL && tq->back[p] == NULL);
}

static uint32_t task_queue_priority_size(task_queue *tq, enum PRIORITY p) {
    uint32_t size = 0;
    task_t *current = tq->front[p];

    while (current != NULL) {
        size += 1;
        current = current->next;
    }
    return size;
}

static task_t *task_queue_pop(task_queue *tq, enum PRIORITY p) {
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
    if (tq->highest_tid == UINT16_MAX) return -1; // out of tids

    tq->highest_tid += 1;
    return tq->highest_tid;
}
