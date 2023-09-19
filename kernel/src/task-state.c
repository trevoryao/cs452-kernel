#include <task-state.h>

#include <util.h>

void task_init(task_t *t, void (*function)(), task_t *parent) {
    memset(t, 0, sizeof(task_t));

    t->pc = (uint64_t)function;
    t->sp = t->stack + STACK_SIZE;

    t->priority = P_MED;
    t->ready_state = STATE_READY;

    t->parent = parent;

    // tid assigned at scheduler
}
