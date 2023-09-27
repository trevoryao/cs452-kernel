#include "task-state.h"

// kernel
#include "context-switch.h"
#include "kassert.h"
#include "task-alloc.h"
#include "task-queue.h"
#include "stack-alloc.h"

// lib
#include "task.h"
#include "syscall.h"
#include "util.h"

// dummy routine to handle end of user function
void user_start(void (*function)(void)) {
    KLOG("user_start called\r\n");
    function();
    Exit();
}

void task_init(task_t *t, void (*function)(), task_t *parent, enum PRIORITY priority, stack_alloc *salloc) {
    t->x0 = (uint64_t)function; // pass param to user_start
    t->x1 = 0;
    t->x2 = 0;
    t->x3 = 0;
    t->x4 = 0;
    t->x5 = 0;
    t->x6 = 0;
    t->x7 = 0;
    t->x8 = 0;
    t->x9 = 0;
    t->x10 = 0;
    t->x11 = 0;
    t->x12 = 0;
    t->x13 = 0;
    t->x14 = 0;
    t->x15 = 0;
    t->x16 = 0;
    t->x17 = 0;
    t->x18 = 0;
    t->x19 = 0;
    t->x20 = 0;
    t->x21 = 0;
    t->x22 = 0;
    t->x23 = 0;
    t->x24 = 0;
    t->x25 = 0;
    t->x26 = 0;
    t->x27 = 0;
    t->x28 = 0;
    t->x29 = 0;
    t->x30 = 0;

    t->pc = (uint64_t)user_start;
    t->sp = (uint64_t)stack_alloc_new(salloc);

    t->pstate = (1 << 7); // mask interrupt

    t->priority = priority;
    t->ready_state = STATE_READY;

    t->parent = parent;

    // tid assigned at scheduler
}

int task_activate(task_t *t, kernel_state *k) {
    context_switch_out(t, k);
    return t->x0;
}

void task_handle(task_t *t, task_alloc *talloc, stack_alloc *salloc, task_queue *tq) {
    // assume syscall (for now)

    KLOG("SYSCALL %x %x %x\r\n", t->x0, t->x1, t->x2);

    switch (t->x0) {
        case SYS_CREAT:
            if (0 <= t->x1 && t->x1 < N_PRIORITY) { // valid priority?
                task_t *new_task;
                if (!(new_task = task_alloc_new(talloc))) {
                    t->x0 = -2; // out of mem
                    break;
                }

                task_init(new_task, (void (*)(void))t->x2, t, t->x1, salloc);

                int32_t tid;
                if ((tid = task_queue_add(tq, new_task)) < 0) {
                    t->x0 = -2; // out of tids
                    break;
                }

                t->x0 = tid; // return new tid
            } else {
                t->x0 = -1; // invalid priority
            }

            break;
        case SYS_TID:
            t->x0 = t->tid;
            break;
        case SYS_PTID:
            if (t->parent) { // has parent
                t->x0 = t->parent->tid;
            } else {
                // killed parent or kernel parent
                t->x0 = 0;
            }
            break;
        case SYS_YIELD:
            break; // let scheduler handle, already moved to back of queue
        case SYS_EXIT:
            task_queue_free_tid(tq, t->tid);
            task_alloc_free(talloc, t);
            break;

        default: kpanic("Unknown syscall number: %d\r\n", t->x0);
    }
}

void task_clear(task_t * t) {
    t->next = NULL;
    t->tid = 0;
}

void kernel_state_init(kernel_state *k) {
    memset(k, 0, sizeof(kernel_state));
}
