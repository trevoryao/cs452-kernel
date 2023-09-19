#include <context-switch.h>
#include <task-state.h>
#include <rpi.h>
#include <syscall.h>
#include <util.h>

void dummy_user(void);

kernel_state *kernel_task;
task_t *curr_user_task;

void kernel_init(void) {
    set_context_switch_handler();

    memset(kernel_task, 0, sizeof(kernel_state));

    task_init(curr_user_task, dummy_user, NULL);
}

int kernel_main(void) {
    kernel_init();

    // context_switch_out();

    return 0;
}

void dummy_user(void) {
    // curr_user_task->x0 = 0;
    // curr_user_task->x1 = 0;
    // curr_user_task->x2 = 0;
    // curr_user_task->x3 = 0;
    // curr_user_task->x4 = 0;
    // curr_user_task->x5 = 0;
    // curr_user_task->x6 = 0;
    // curr_user_task->x7 = 0;
    // curr_user_task->x8 = 0;
    // curr_user_task->x9 = 0;
    // curr_user_task->x10 = 0;
    // curr_user_task->x11 = 0;
    // curr_user_task->x12 = 0;
    // curr_user_task->x13 = 0;
    // curr_user_task->x14 = 0;
    // curr_user_task->x15 = 0;
    // curr_user_task->x16 = 0;
    // curr_user_task->x17 = 0;
    // curr_user_task->x18 = 0;
    // curr_user_task->x19 = 0;
    // curr_user_task->x20 = 0;
    // curr_user_task->x21 = 0;
    // curr_user_task->x22 = 0;
    // curr_user_task->x23 = 0;
    // curr_user_task->x24 = 0;
    // curr_user_task->x25 = 0;
    // curr_user_task->x26 = 0;
    // curr_user_task->x27 = 0;
    // curr_user_task->x28 = 0;
    // curr_user_task->x29 = 0;
    curr_user_task->x30 = 0;
    // curr_user_task->pc = 0;
    // curr_user_task->sp = 0;
    // curr_user_task->pstate = 0;

    //syscall(100, 10, 20, 0, 0, 0, 0, 0);
}
