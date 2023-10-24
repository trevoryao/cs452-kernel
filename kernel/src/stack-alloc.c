#include "stack-alloc.h"

// kernel
#include "kassert.h"

// lib
#include "util.h"

void stack_alloc_init(stack_alloc *salloc, void *kernel_stack_ptr) {
    salloc->base = (char *)kernel_stack_ptr + STACK_SIZE;
    memset(salloc->is_allocd, 0, N_TASK_T * N_SLABS);
}

void *stack_alloc_new(stack_alloc *salloc) {
    uint16_t offset = 0;

    // find first free offset
    while ((offset < N_TASK_T * N_SLABS) && salloc->is_allocd[offset]) ++offset;

    // since N_TASK_T is finite upper limit, can assume that there WILL be a free offset
    // never hurts to be safe regardless
    kassert(offset < N_TASK_T * N_SLABS);

    salloc->is_allocd[offset] = 1;

    return (char *)salloc->base + (offset * STACK_SIZE);
}

void stack_alloc_free(stack_alloc *salloc, void *stack_base) {
    uint16_t offset = (((char *)stack_base - (char *)salloc->base) / STACK_SIZE);
    kassert(offset < N_TASK_T * N_SLABS);
    salloc->is_allocd[offset] = 0;
}
