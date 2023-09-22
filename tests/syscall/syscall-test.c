#include <context-switch.h>
#include <kassert.h>
#include <stack-alloc.h>
#include <task-state.h>

#include <rpi.h>
#include <syscall.h>
#include <util.h>

void dummy_user(void);

kernel_state *kernel_task;
task_t *curr_user_task;

void kernel_init(stack_alloc *salloc) {
    init_exception_handlers();

    uart_init();
    uart_config_and_enable(CONSOLE, 115200, 1);

    uart_puts(CONSOLE, "\033[2J\033[H"); // clear screen
    uart_printf(CONSOLE, "Niclas Heun & Trevor Yao: syscall-test (%s)\r\n", __TIME__);

    task_init(curr_user_task, dummy_user, NULL, P_MED, salloc);
}

int kernel_main(void *kernel_end) {
    kernel_state k;
    kernel_task = &k;

    task_t t;
    curr_user_task = &t;

    stack_alloc salloc;
    stack_alloc_init(&salloc, kernel_end);

    kernel_init(&salloc);

    uart_printf(CONSOLE, "addr of user_task f'n: %x\r\n", dummy_user);

    uart_printf(CONSOLE, "activating user task\r\n");
    print_el();

    task_activate(curr_user_task);
    uart_printf(CONSOLE, "request number %d\r\n", curr_user_task->x0);
    print_el();

    while (!uart_out_empty(CONSOLE)); // spin until output finishes
    return 0;
}

void dummy_user(void) {
    uart_printf(CONSOLE, "hello world\r\n");
    // print_el();
    syscall(10, 20, 30, 0, 0, 0, 0, 0);
}
