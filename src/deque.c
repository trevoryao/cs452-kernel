#include "deque.h"

#include "util.h"

int deque_init(deque *q, uint16_t size) {
    if (size > 10) return -1; // check over 1024 limit

    q->size = 0;
    q->max_size = 1 << size; // 2^size

    q->front = 0;
    q->back = 0;

    // don't want to, but can cause issues on boot up for some reason
    memset(q->buf, 0, BUF_SIZE * sizeof(int32_t));
    return 0;
}

void deque_reset(deque *q) {
    q->size = 0;
    q->front = 0;
    q->back = 0;
}

void deque_move(deque *q, deque *other) {
    q->size = 0;
    q->max_size = other->max_size;

    // don't want to, but can cause issues on boot up for some reason
    memset(q->buf, 0, BUF_SIZE * sizeof(int32_t));

    while (!deque_empty(other)) {
        deque_push_back(q, deque_pop_front(other));
    }
}

int16_t deque_empty(deque *q) { return q->size == 0; }
int16_t deque_full(deque *q) { return q->size == q->max_size; }
uint16_t deque_size(deque *q) { return q->size; }

void deque_push_back(deque *q, int32_t x) {
    if (deque_full(q)) return; // drop incoming
    q->buf[q->back] = x;
    q->back = (q->back + 1) & (q->max_size - 1); // mod q->max_size
    ++q->size;
}

int32_t deque_pop_back(deque *q) {
    if (deque_empty(q)) return -1;

    int32_t retval = deque_back(q);
    q->back = (q->back + q->max_size - 1) & (q->max_size - 1); // mod q->max_size
    --q->size;
    return retval;
}

int32_t deque_back(deque *q) {
    return q->buf[(q->back + q->max_size - 1) & (q->max_size - 1)]; // mod q->max_size
}

void deque_push_front(deque *q, int32_t x) {
    if (deque_full(q)) return; // drop incoming
    q->front = (q->front + q->max_size - 1) & (q->max_size - 1); // mod q->max_size
    q->buf[q->front] = x;
    ++q->size;
}

int32_t deque_pop_front(deque *q) {
    if (deque_empty(q)) return -1;

    int32_t retval = deque_front(q);
    q->front = (q->front + 1) & (q->max_size - 1); // mod q->max_size
    --q->size;
    return retval;
}

int32_t deque_front(deque *q) { return q->buf[q->front]; }

int32_t deque_itr_get(deque *q, deque_itr itr) {
    return q->buf[itr & (q->max_size - 1)]; // mod q->max_size
}

deque_itr deque_begin(deque *q) { return q->front; }

deque_itr deque_end(deque *q) {
    return (q->front == q->back) ? (q->back + q->max_size) : q->back;
}

deque_itr deque_itr_next(deque_itr itr) {
    return itr + 1;
}
