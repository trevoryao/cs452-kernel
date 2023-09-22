#include <task-state.h>

#include <context-switch.h>
#include <kassert.h>
#include <stack-alloc.h>

#include <task.h>
#include <syscall.h>
#include <util.h>

// dummy routine to handle end of user function
void user_start(void (*function)(void)) {
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

    t->pstate = (1 << 7); // mask interrupt

    t->pc = (uint64_t)user_start;
    t->sp = (uint64_t)stack_alloc_new(salloc);

    t->priority = priority;
    t->ready_state = STATE_READY;

    t->parent = parent;

    // tid assigned at scheduler
}

int task_activate(task_t *t) {
    context_switch_out();
    return t->x0;
}

void task_handle(task_t *t) {
    // assume syscall (for now)

    switch (t->x0) {
        case SYS_CREAT:
            // cast x2 as func ptr
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
            t->x0 = 0;
            break; // let scheduler handle, already moved to back of queue
        case SYS_EXIT:
            t->ready_state = STATE_KILLED; // let scheduler handle
            break;

        default: kpanic("Unknown syscall number: %d", t->x0);
    }
}

void task_clear(task_t * t) {
    memset(t, 0, sizeof(task_t));
}

void kernel_state_init(kernel_state *k) {
    memset(k, 0, sizeof(kernel_state));
}
