#include "rpi.h"
#include "priority-queue.h"
#include "uassert.h"

void output_heap(priority_queue *pq) {
    int level_max = 1;
    int level = 0;
    for (int i = 0; i < pq->size; ++i, ++level) {
        if (level == level_max) {
            uart_puts(CONSOLE, "\r\n");
            level_max *= 2;
            level = 0;
        }

        uart_printf(CONSOLE, "(%d) %d\t", pq->heap[i].priority, pq->heap[i].value);
    }

    uart_printf(CONSOLE, "\r\n");
}

void user_main(void) {
    // setup queue
    priority_queue pq;
    pq.size = 10;

    for (int i = 0; i < pq.size; ++i) {
        pq.heap[i].priority = 2 * i;
        pq.heap[i].value = 4 * i;
    }

    priority_queue_init(&pq);

    uart_printf(CONSOLE, "initial:\r\n");
    output_heap(&pq);

    priority_queue_add(&pq, 10, 5);
    uart_printf(CONSOLE, "add 5:\r\n");
    output_heap(&pq);

    priority_queue_add(&pq, 6, 3);
    uart_printf(CONSOLE, "add 3:\r\n");
    output_heap(&pq);

    int popped;
    for (int i = 0; i < 5; ++i) {
        popped = priority_queue_pop_min(&pq);
        uart_printf(CONSOLE, "pop %d:\r\n", popped);
        output_heap(&pq);
    }

    priority_queue_decrease(&pq, 36, 5);
    uart_printf(CONSOLE, "adjust 36:\r\n");
    output_heap(&pq);

    priority_queue_decrease(&pq, 32, 0);
    uart_printf(CONSOLE, "adjust 32:\r\n");
    output_heap(&pq);
}
