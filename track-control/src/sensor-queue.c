#include "sensor-queue.h"

#include <stdint.h>
#include "util.h"
#include "rpi.h"


void sensor_queue_init(sensor_queue *sq) {
    memset(sq->storage, 0, sizeof(struct sensor_queue_entry) * MAX_WAITING_PROCESSES);

    for (int i = 0; i < N_SENSOR_MODULES; ++i) {
        memset(sq->sensors[i], 0, sizeof(sensor_queue_entry*) * N_SENSORS);

    }
    sq->freelist = NULL;

    for (int i = 0; i < MAX_WAITING_PROCESSES; i++) {
        sq->storage[i].next = sq->freelist;
        sq->freelist = &sq->storage[i];
    }

}

// returns if any task is waiting for this specific module
bool sensor_queue_empty(sensor_queue *sq, uint16_t sensor_mod, uint16_t sensor_no) {
    int act_sensor_mod = sensor_mod - 1;
    int act_sensor_no = sensor_no - 1;
    return (sq->sensors[act_sensor_mod][act_sensor_no] == NULL);
}

// returns first waiting tid
uint16_t sensor_queue_get_waiting_tid(sensor_queue *sq, uint16_t sensor_mod, uint16_t sensor_no) {
    int act_sensor_mod = sensor_mod - 1;
    int act_sensor_no = sensor_no - 1;
    if (sq->sensors[act_sensor_mod][act_sensor_no] == NULL) {
        return 0;
    } else {
        struct sensor_queue_entry *head = sq->sensors[act_sensor_mod][act_sensor_no];
        uint16_t tid = head->tid;

        // set queue to next position
        sq->sensors[act_sensor_mod][act_sensor_no] = head->next;

        // reset the queue entry and add to free list
        head->next = NULL;
        head->tid = 0;

        head->next = sq->freelist;
        sq->freelist = head;

        return tid;
    }
}

// add a process to the waiting list
void sensor_queue_add_waiting_tid(sensor_queue *sq, uint16_t sensor_mod, uint16_t sensor_no, uint16_t tid) {
    int act_sensor_mod = sensor_mod - 1;
    int act_sensor_no = sensor_no - 1;

    if (act_sensor_mod >= N_SENSOR_MODULES  || act_sensor_no >= N_SENSORS) {
        uart_printf(CONSOLE, "Out of bound - wrong sensor number\r\n");
        return;

    }

    
    if (sq->freelist == NULL) {
        // TODO: error message
        uart_printf(CONSOLE, "Cannot enqueu\r\n");
        return;
    } else {
        // get new element
        //uart_printf(CONSOLE, "Enqueue in queue to tid %d sensor_module %d sensor_no %d\r\n", tid, sensor_mod, sensor_no);
        struct sensor_queue_entry *element = sq->freelist;
        sq->freelist = sq->freelist->next;

        element->next = NULL;
        element->tid = tid;

        // add it to the list
        element->next = sq->sensors[act_sensor_mod][act_sensor_no];
        sq->sensors[act_sensor_mod][act_sensor_no] = element;

        //uart_printf(CONSOLE, "Enqueued to array %d\r\n", sq->sensors[act_sensor_mod][act_sensor_no]->tid);
        //uart_printf(CONSOLE, "Enqueued to array with array pos %d\r\n", sq->sensors[act_sensor_mod][act_sensor_no]);
    }
}