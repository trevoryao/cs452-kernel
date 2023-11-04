#include "priority-queue.h"

#include "stddef.h"

#define LEFT(i) (((i) * 2) + 1)
#define RIGHT(i) (((i) * 2) + 2)
#define PARENT(i) (((i) - 1) / 2)

static void heapify(priority_queue *pq, int16_t i) {
    int16_t left = LEFT(i);
    int16_t right = RIGHT(i);
    int16_t min = i;

    if (left >= pq->size || left < 0)
        left = -1;
    if (right >= pq->size || right < 0)
        right = -1;

    if (left != -1 && pq->heap[left].priority < pq->heap[i].priority)
        min = left;
    if (right != -1 && pq->heap[right].priority < pq->heap[min].priority)
        min = right;

    if (min != i) { // need to swap?
        // swap min and i nodes
        void *tmp;

        tmp = pq->heap[min].value;
        pq->heap[min].value = pq->heap[i].value;
        pq->heap[i].value = tmp;

        tmp = pq->heap[min].priority;
        pq->heap[min].priority = pq->heap[i].priority;
        pq->heap[i].priority = tmp;

        heapify(pq, min);
    }
}

void priority_queue_init(priority_queue *pq) {
    // also assume size is fine
    for (int i = (pq->size >> 1) - 1; i >= 0; --i)
        heapify(pq, i);
}

static void priority_queue_bubble_up(priority_queue *pq, int16_t i) {
    int16_t parent = PARENT(i);

    if (pq->heap[parent].priority > pq->heap[i].priority) {
        // swap parent and i nodes
        int16_t tmp;

        tmp = pq->heap[parent].value;
        pq->heap[parent].value = pq->heap[i].value;
        pq->heap[i].value = tmp;

        tmp = pq->heap[parent].priority;
        pq->heap[parent].priority = pq->heap[i].priority;
        pq->heap[i].priority = tmp;

        priority_queue_bubble_up(pq, parent);
    }
}

void priority_queue_add(priority_queue *pq, void *val, int16_t priority) {
    if (pq->size < MAX_PQ_ELEMENTS) {
        pq->heap[pq->size].value = val;
        pq->heap[pq->size].priority = priority;
        priority_queue_bubble_up(pq, pq->size);
        ++pq->size;
    }
}

void *priority_queue_pop_min(priority_queue *pq) {
    if (pq->size == 0) return NULL;

    void *retval = pq->heap[0].value;

    // replace top with last node
    pq->heap[0].value = pq->heap[pq->size - 1].value;
    pq->heap[0].priority = pq->heap[pq->size - 1].priority;

    --pq->size;

    heapify(pq, 0);
    return retval;
}

bool priority_queue_decrease(priority_queue *pq, void *val,
    int16_t new_priority) {
    // find idx
    int16_t i = 0;
    while (i < pq->size && pq->heap[i].value != val) ++i;
    if (i == pq->size) return false; // not present

    pq->heap[i].priority = new_priority;

    priority_queue_bubble_up(pq, i);
    return true;
}

bool priority_queue_empty(priority_queue *pq) {
    return pq->size == 0;
}
