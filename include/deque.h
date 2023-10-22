#ifndef __DEQUE_H__
#define __DEQUE_H__

#include <stdint.h>

/*
 * Deque implemented as a circular buffer.
 * Can be used as a stack or a deque (or even both).
 * Max size is 1024 * sizeof(int16_t) = 4096 bytes
 * For efficiency assumes size is a power of 2
*/

#define BUF_SIZE 1025

typedef struct deque {
    uint16_t size; // cur size
    uint16_t max_size;
    int16_t front, back;

    int32_t buf[BUF_SIZE]; // extra black space for iterators
} deque;

// where size is the exponent of 2. For example, if we
// want a deque of maximum size 8, pass size = 3.
// maximum possible is size = 11 (2048)
int deque_init(deque *q, uint16_t size);
void deque_reset(deque *q);

int16_t deque_empty(deque *q);
int16_t deque_full(deque *q);
uint16_t deque_size(deque *q);

void deque_push_back(deque *q, int32_t x);
int32_t deque_pop_back(deque *q);
int32_t deque_back(deque *q);

void deque_push_front(deque *q, int32_t x);
int32_t deque_pop_front(deque *q);
int32_t deque_front(deque *q);

// iterator
typedef int16_t deque_itr;

deque_itr deque_begin(deque *q);
deque_itr deque_end(deque *q);

int32_t deque_itr_get(deque *q, deque_itr itr);
deque_itr deque_itr_next(deque_itr itr);

#endif
