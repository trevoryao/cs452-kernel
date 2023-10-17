#include "clock-queue.h"

void clock_queue_init(clock_queue *cq) {
    // zero all entries
    memset(cq->storage, 0, sizeof(clock_queue_entry) * MAX_WAITING_TASKS);

    // zero the free list
    cq->freelist = NULL;
    cq->head = NULL;

    // put all entries to the freelist
    for (int i = 0; i < MAX_WAITING_TASKS; i++) {
        cq->storage[i].next = cq->freelist;
        cq->freelist = &cq->storage[i];
    }
}

// returns if any task can be returned
bool clock_queue_ready(clock_queue *cq, uint64_t curr_clock_tick) {
    if (cq->head == NULL) {
        return false;
    } else {
        return (cq->head->clock_tick < curr_clock_tick);
    }
}

// returns first element that is ready
uint16_t clock_queue_get(clock_queue *cq, uint64_t curr_clock_tick) {
    if ((cq->head == NULL) || (cq->head->clock_tick > curr_clock_tick)) {
        return 0;
    } else {
        clock_queue_entry *elem = cq->head;
        uint16_t tid = elem->tid;

        // pop off queue and add to free list
        cq->head = cq->head->next;
        elem->next = cq->freelist;
        cq->freelist = elem;

        return tid;
    }
}

// add a new entry sorted to the list
void clock_queue_add(clock_queue *cq, uint16_t tid, uint64_t clock_tick) {
    // TODO: Error handling if queue is exhausted
    if (cq->freelist == NULL) {
        return;
    }

    // create a new entry
    clock_queue_entry *new_element = cq->freelist;
    cq->freelist = cq->freelist->next;

    new_element->clock_tick = clock_tick;
    new_element->tid = tid;
    new_element->next = NULL;

    // sorted add to the list

    // Case 1: insert at head of the list
    if (cq->head == NULL || cq->head->clock_tick > new_element->clock_tick) {
        new_element->next = cq->head;
        cq->head = new_element;
    } else {
        // go through the list to add it to the correct position
        clock_queue_entry *current;
        current = cq->head;

        while (current->next != NULL && (current->next->clock_tick < new_element->clock_tick)) {
            current = current->next;
        }

        new_element->next = current->next;
        current->next = new_element;
    }
}
