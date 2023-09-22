#include <context-switch.h>
#include <task-state.h>
#include <rpi.h>
#include <syscall.h>

void dummy_user(void);

void kernel_init(void) {
    set_context_switch_handler();
}

int kernel_main(void) {
    kernel_init();

    dummy_user();

    return 0;
}

void dummy_user(void) {
    syscall(100, 8, 2, 0, 0, 0, 0, 0);
}
