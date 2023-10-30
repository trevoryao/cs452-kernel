#include "sensor-queue.h"

#include <stdint.h>
#include "util.h"


void sensor_queue_init(sensor_queue *sq) {
    memset(sq->storage, 0, sizeof(struct sensor_queue_entry) * MAX_WAITING_PROCESSES);
    memset(sq->sensors, 0, sizeof(sensor_queue_entry*) * N_SENSOR_MODULES * N_SENSORS);

    sq->freelist = NULL;
}

// returns if any task is waiting for this specific module
bool sensor_queue_empty(sensor_queue *sq, uint16_t sensor_mod, uint16_t sensor_no) {
    return (sq->sensors[sensor_mod][sensor_no] == NULL);
}

// returns first waiting tid
uint16_t sensor_queue_get_waiting_tid(sensor_queue *sq, uint16_t sensor_mod, uint16_t sensor_no) {
    if (sq->sensors[sensor_mod][sensor_no] == NULL) {
        return 0;
    } else {
        struct sensor_queue_entry *head = sq->sensors[sensor_mod][sensor_no];
        uint16_t tid = head->tid;

        // set queue to next position
        sq->sensors[sensor_mod][sensor_no] = head->next;

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
    if (sq->freelist == NULL) {
        // TODO: error message       
        return;
    } else {
        // get new element
        struct sensor_queue_entry *element = sq->freelist;
        sq->freelist = sq->freelist->next;

        element->next = NULL;
        element->tid = tid;

        // add it to the list
        element->next = sq->sensors[sensor_mod][sensor_no];
        sq->sensors[sensor_mod][sensor_no] = element;
    }
}