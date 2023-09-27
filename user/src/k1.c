#include "k1.h"

#include <stdint.h>

// lib
#include "rpi.h"
#include "task.h"

/*
User Tasks

The following user tasks test your kernel.

First User Task

The first user task should create four instances of a test task.
Two should be at a priority lower than the priority of the first user task.
Two should be at a priority higher than the priority of the first user task.
The lower priority tasks should be created before the higher priority tasks.
On return of each Create(), busy-wait IO should be used to print "Created: <task id>" on the terminal screen.
After creating all tasks the first user task should call Exit(), immediately after printing "FirstUserTask: exiting".

The Other Tasks

The tasks created by the first user task have the same code, but possibly different data.
They first print a line with their task id and their parent's task id.
They call Yield().
They print the same line a second time.
Finally they call Exit().

 */

void func(void) {
    int tid = MyTid();
    int parent_tid = MyParentTid();

    uart_printf(CONSOLE, "Task %d parent: %d\r\n", tid, parent_tid);
    Yield();
    uart_printf(CONSOLE, "Task %d parent: %d\r\n", tid, parent_tid);
    // Exit called implicitly
}

void kernel1_test(void) {
    for (uint8_t i = 0; i < 2; ++i) {
        int tid = Create(P_LOW, func);
        uart_printf(CONSOLE, "Created: %d\r\n", tid);
    }

    for (uint8_t i = 0; i < 2; ++i) {
        int tid = Create(P_HIGH, func);
        uart_printf(CONSOLE, "Created: %d\r\n", tid);
    }

    uart_printf(CONSOLE, "FirstUserTask: exiting\r\n");
    // Exit called implicitly
}
