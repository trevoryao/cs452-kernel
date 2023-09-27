#include <context-switch.h>
#include <kassert.h>
#include <stack-alloc.h>
#include <task-state.h>

#include <rpi.h>
#include <syscall.h>
#include <term-control.h>
#include <util.h>

void dummy_user(void);

// global state for context switching

void kernel_init(kernel_state *kernel_task, task_t *curr_user_task, stack_alloc *salloc, void *kernel_end) {
    // initialise singletons
    stack_alloc_init(salloc, kernel_end);

    // set global state
    kernel_state_init(kernel_task);

    init_exception_handlers();

    uart_init();
    uart_config_and_enable(CONSOLE, 115200, 1);

    // some initialisation display
    uart_puts(CONSOLE, CLEAR CURS_START);
    uart_printf(CONSOLE, "Niclas Heun & Trevor Yao: Syscall Test (%s)\r\n", __TIME__);

    // initialise first user task
    task_init(curr_user_task, dummy_user, NULL, P_MED, salloc);
}

int kernel_main(void *kernel_end) {
    kernel_state kernel_task;     // kernel state for context switching
    task_t curr_user_task;
    stack_alloc salloc;

    kernel_init(&kernel_task, &curr_user_task, &salloc, kernel_end);

    uart_printf(CONSOLE, "addr of user_task f'n: %x\r\n", dummy_user);

    uart_printf(CONSOLE, "activating user task\r\n");
    print_el();

    task_activate(&curr_user_task, &kernel_task);
    uart_printf(CONSOLE, "request number %d\r\n", curr_user_task.x0);
    print_el();

    while (!uart_out_empty(CONSOLE)); // spin until output finishes
    return 0;
}

void dummy_user(void) {
    uart_printf(CONSOLE, "hello user world\r\n");
    // print_el();
    syscall(10, 20, 30, 0, 0, 0, 0, 0);
}

void task_test(task_t *curr_user_task, kernel_state *kernel_task) {
    curr_user_task->pc = 0;
    curr_user_task->sp = 0;
    curr_user_task->pstate = 0;
}
