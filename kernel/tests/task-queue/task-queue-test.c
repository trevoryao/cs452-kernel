#include "task-queue.h"
#include "task-state.h"
#include "task-alloc.h"

// #include "rpi.h"
#include <assert.h>

kernel_state *kernel_task = NULL; // needed to satisfy linker
task_t *curr_user_task = NULL;
task_t *user_task0 = NULL;
task_t *user_task1 = NULL;
task_t *user_task2 = NULL;
task_t *user_task3 = NULL;
task_t *user_task4 = NULL;
task_t *user_task5 = NULL;

// Some function to nicely print the test success/failure
void print_test_result(int test_number, bool successfull) {
    // if (successfull) {
    //     uart_printf(CONSOLE, "Test %u was successful\r\n", test_number);
    // } else {
    //     uart_printf(CONSOLE, "Test %u failed\r\n", test_number);
    // }

    assert(successfull);
}

int main(void) {
    task_alloc t_allocator;
    task_alloc_init(&t_allocator);
    task_queue t_queue;
    task_queue_init(&t_queue);

    /*
        Testing the task_alloc
    */

    // Test 1: Getting a new task
    user_task1 = task_alloc_new(&t_allocator);
    print_test_result(1, user_task1 != NULL);

    // Test 2: Freeing the memory should add it to the free list (->next pointer should not be null)
    task_alloc_free(&t_allocator, user_task1);
    print_test_result(2, user_task1->next != NULL);


    /*
        Testing the task_queue
    */
    user_task1 = task_alloc_new(&t_allocator);
    user_task2 = task_alloc_new(&t_allocator);
    user_task1->priority = P_MED;
    user_task2->priority = P_MED;
    user_task1->ready_state = STATE_READY;
    user_task2->ready_state = STATE_READY;

    // Test 3: Adding task to queue and it gets tid
    task_queue_add(&t_queue, user_task1);
    task_queue_add(&t_queue, user_task2);
    print_test_result(3, (user_task1->tid != 0 && user_task1->tid < user_task2->tid));

    // Test 4: trying to schedule a task
    curr_user_task = task_queue_schedule(&t_queue);
    print_test_result(4, (curr_user_task == user_task1));

    // Test 5: putting user_task1 back to queue with READY & setting task2 to not ready
    user_task1->ready_state = STATE_READY;
    user_task2->ready_state = STATE_BLOCKED;

    task_queue_add(&t_queue, user_task1);

    curr_user_task = task_queue_schedule(&t_queue);
    print_test_result(5, (curr_user_task == user_task1));

    // Test 6: Testing with different priorities
    user_task2->ready_state = STATE_BLOCKED;
    user_task1->priority = P_VLOW;
    task_queue_add(&t_queue, user_task1);
    curr_user_task = task_queue_schedule(&t_queue);
    print_test_result(6, (curr_user_task == user_task1));

    // for (;;);
    return 0;
}
