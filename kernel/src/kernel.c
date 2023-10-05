// kernel
#include "context-switch.h"
#include "kassert.h"
#include "stack-alloc.h"
#include "task-alloc.h"
#include "task-queue.h"
#include "task-state.h"

// lib
#include "nameserver.h"
#include "rpi.h"
#include "syscall.h"
#include "task.h"
#include "term-control.h"
#include "util.h"

extern void user_main(void); // predefined user-level starting point

void kernel_init(kernel_state *kernel_task, task_t *curr_user_task, task_alloc *talloc, stack_alloc *salloc, void *kernel_end, task_queue *tqueue) {
    // initialise singletons
    task_alloc_init(talloc);
    stack_alloc_init(salloc, kernel_end);
    task_queue_init(tqueue);

    // set global state
    kernel_state_init(kernel_task);
    curr_user_task = task_alloc_new(talloc);

    init_exception_handlers();

    uart_init();
    uart_config_and_enable_console();
    uart_config_and_enable_marklin();

    // some initialisation display
    uart_puts(CONSOLE, CLEAR CURS_START);
    uart_printf(CONSOLE, "Niclas Heun & Trevor Yao: CS 452 Kernel (%s)\r\n", __TIME__);

    // initialise nameserver
    task_t *ns = task_alloc_new(talloc);
    task_init(ns, nameserver_main, NULL, P_SERVER, salloc);
    ns->tid = NAMESERVER_TID; // explicitly set TID
    task_queue_add(tqueue, ns);

    // initialise first user task
    task_init(curr_user_task, user_main, NULL, P_MED, salloc);
    task_queue_add(tqueue, curr_user_task);
}

int kernel_main(void *kernel_end) {
    kernel_state kernel_task;     // kernel state for context switching
    task_t *curr_user_task = NULL;
    task_alloc talloc;
    stack_alloc salloc;
    task_queue tqueue;

    kernel_init(&kernel_task, curr_user_task, &talloc, &salloc, kernel_end, &tqueue);

    for (;;) {
        curr_user_task = task_queue_schedule(&tqueue);

        KLOG("scheduling task-%d (%x)\r\n", curr_user_task->tid, curr_user_task);

        task_activate(curr_user_task, &kernel_task);
        int exited = task_handle(curr_user_task, &talloc, &salloc, &tqueue);
        if (!exited) task_queue_add(&tqueue, curr_user_task);

        curr_user_task = NULL; // drop ownership

        if (task_queue_empty(&tqueue)) {
            KLOG("All Tasks Exited\r\n");
            break; // EXIT: no more running tasks
        }
    }

    uart_puts(CONSOLE, "\r\n");
    while (!uart_out_empty(CONSOLE)); // spin until output finishes
    return 0;
}
