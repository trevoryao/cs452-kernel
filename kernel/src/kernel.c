// kernel
#include "context-switch.h"
#include "event-queue.h"
#include "interrupts.h"
#include "kassert.h"
#include "stack-alloc.h"
#include "stopwatch.h"
#include "task-alloc.h"
#include "task-queue.h"
#include "task-state.h"

// lib
#include "nameserver.h"
#include "rpi.h"
#include "syscall.h"
#include "sys-clock.h"
#include "task.h"
#include "term-control.h"
#include "time.h"
#include "uassert.h"
#include "util.h"

extern void user_main(void); // predefined user-level starting point

static void idle_task_main(void) { for (;;) asm("wfi"); } // low-power idle task main

static void kernel_init(kernel_state *kernel_task, task_t *curr_user_task,
    uint16_t *idle_task_tid, task_alloc *talloc, stack_alloc *salloc, void *kernel_end,
    task_queue *tqueue, event_queue *equeue, stopwatch *stopwatch) {
    // initialise singletons
    task_alloc_init(talloc);
    stack_alloc_init(salloc, kernel_end);
    task_queue_init(tqueue);
    event_queue_init(equeue);
    stopwatch_init(stopwatch);

    // set global state
    kernel_state_init(kernel_task);
    curr_user_task = task_alloc_new(talloc);

    uart_init();
    uart_config_and_enable_console();
    uart_config_and_enable_marklin();

    // some initialisation display
    uart_puts(CONSOLE, CLEAR CURS_START);
    uart_printf(CONSOLE, "Niclas Heun & Trevor Yao: CS 452 Kernel (%s %s)\r\n", __DATE__, __TIME__);

    init_exception_handlers();
    init_interrupt_handlers();

    // initialise nameserver
    task_t *ns = task_alloc_new(talloc);
    task_init(ns, nameserver_main, NULL, P_SERVER_LO, salloc);
    ns->tid = NAMESERVER_TID; // explicitly set TID
    task_queue_add(tqueue, ns);

    // initialise first user task
    task_init(curr_user_task, user_main, NULL, P_MED, salloc);
    task_queue_add(tqueue, curr_user_task);

    // initialise idle task
    task_t *idle_task = task_alloc_new(talloc);
    task_init(idle_task, idle_task_main, NULL, P_IDLE, salloc);
    task_queue_add(tqueue, idle_task);
    *idle_task_tid = idle_task->tid; // save tid for timing purposes
}

int kernel_main(void *kernel_end) {
    kernel_state kernel_task;     // kernel state for context switching
    task_t *curr_user_task = NULL;
    uint16_t idle_task_tid;
    task_alloc talloc;
    stack_alloc salloc;
    task_queue tqueue;
    event_queue equeue;
    stopwatch stopwatch;

    kernel_init(&kernel_task, curr_user_task, &idle_task_tid,
        &talloc, &salloc, kernel_end, &tqueue, &equeue, &stopwatch);

    for (;;) {
        curr_user_task = task_queue_schedule(&tqueue);

        if (curr_user_task->ready_state == STATE_KILLED) {
            goto CURR_TASK_KILLED;
        } else { // ready
            curr_user_task->ready_state = STATE_RUNNING;
        }

        enum STPWS stpw_t = (curr_user_task->tid == idle_task_tid) ? STPW_IDLE_TASK : STPW_USER_TASK;
        stopwatch_start(&stopwatch, stpw_t);

        uint8_t handler = task_activate(curr_user_task, &kernel_task);

        stopwatch_end(&stopwatch, stpw_t);

        if ((handler & SYNC_MSK) == SYNC_MSK) { // syscall?
            task_svc_handle(curr_user_task, &talloc, &salloc,
                &tqueue, &equeue, &stopwatch);
        } else if ((handler & IRQ_MSK) == IRQ_MSK) { // interrupt?
            handle_interrupt(&equeue);
        } else {
            // unimplemented handler
            kpanic("switched back from unimplemented handler %u????\n", handler);
        }

        if (curr_user_task->ready_state == STATE_KILLED) {
CURR_TASK_KILLED:
            task_dealloc(curr_user_task, &talloc, &salloc, &tqueue);
        } else {
            task_queue_add(&tqueue, curr_user_task);
        }
        curr_user_task = NULL; // drop ownership

        if (task_queue_empty(&tqueue)) {
            KLOG("All Tasks Exited\r\n");
            break; // EXIT: no more running tasks
        }
    }

    // print out timer stats
    time_t idle_time;
    uint64_t idle_ticks = stopwatch_get_total_ticks(&stopwatch, STPW_IDLE_TASK);
    uint64_t user_ticks = stopwatch_get_total_ticks(&stopwatch, STPW_USER_TASK);

    time_from_sys_ticks(&idle_time, idle_ticks);
    int idle_prop = (idle_ticks * 100) / (idle_ticks + user_ticks);

    uart_printf(CONSOLE, "Total idle time: %u:%u.%u (%d%%)\r\n", idle_time.min, idle_time.sec, idle_time.tsec, idle_prop);

    return 0;
}
