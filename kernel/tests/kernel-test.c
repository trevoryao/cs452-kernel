#include "task-queue.h"
#include "task-state.h"
#include "task-memory.h"

#include <assert.h>

task_alloc t_allocator;
task_queue t_queue;
task_t *current_task = NULL;
task_t *user_task1 = NULL;
task_t *user_task2 = NULL;

// #define SAVE_CURSOR_POS         "\033[s"
// #define RESTORE_CURSOR          "\033[u"
// #define DELETE_REST_OF_LINE     "\033[K"
// #define BASE_CURSOR             0


void init(void) {
    // Will only work after merge
    task_alloc_init(&t_allocator);
    task_queue_init(&t_queue);
}

int main(void) {
    init();
    /*
        Testing the task_alloc
    */
    // Test 1: Getting a new task
    user_task1 = task_alloc_new(&t_allocator);
    //print_test_result(1, user_task1 != NULL);
    assert(user_task1 != NULL);

    // Test 2: Freeing the memory should add it to the free list (->next pointer should not be null)
    task_alloc_free(&t_allocator, user_task1);
    //print_test_result(2, user_task1->next != NULL);
    assert(user_task1->next != NULL);
    
    
    
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
    //print_test_result(3, (user_task1->tid != 0 && user_task1->tid < user_task2->tid));
    assert((user_task1->tid != 0 && user_task1->tid < user_task2->tid));

    // Test 4: trying to schedule a task
    current_task = task_queue_schedule(&t_queue);
    //print_test_result(4, (current_task == user_task1));
    assert(current_task == user_task1);

    // Test 5: putting user_task1 back to queue with READY & setting task2 to not ready
    user_task1->ready_state = STATE_READY;
    user_task2->ready_state = STATE_BLOCKED;

    task_queue_add(&t_queue, user_task1);
    
    current_task = task_queue_schedule(&t_queue);
    //print_test_result(5, (current_task == user_task1));
    assert(current_task == user_task1);

    // Test 6: Testing with different priorities
    user_task2->ready_state = STATE_BLOCKED;
    user_task1->priority = P_VLOW;
    task_queue_add(&t_queue, user_task1);
    current_task = task_queue_schedule(&t_queue);
    //print_test_result(6, (current_task == user_task1));
    assert(current_task == user_task1);

    return 0;
}

// // Some function to nicely print the test success/failure
// void print_test_result(int test_number, bool successfull) {
//     int cursor_pos = BASE_CURSOR + test_number;
//     uart_printf(CONSOLE, "%s", SAVE_CURSOR_POS);
//     if (successfull) {
//         uart_printf(CONSOLE, "\033[%u;0H%s Test %u was successful", cursor_pos, DELETE_REST_OF_LINE, cursor_pos);
//     } else {
//         uart_printf(CONSOLE, "\033[%u;0H%s Test %u failed", cursor_pos, DELETE_REST_OF_LINE, cursor_pos);
//     }
//     uart_printf(CONSOLE, "%s", RESTORE_CURSOR);
// }
