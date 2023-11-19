#include "sensor-queue.h"

#include "uassert.h"
#include "util.h"

#include "controller-consts.h"
#include "track-control.h"

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

static void
sensor_queue_insert(sensor_queue *sq, uint16_t act_sensor_mod,
    uint16_t act_sensor_no, struct sensor_queue_entry *element) {
    // insert in sorted order based on expected time
    struct sensor_queue_entry *head = sq->sensors[act_sensor_mod][act_sensor_no];

    // case 1: insert at beginning
    if (head == NULL ||
        head->data.expected_time > element->data.expected_time) {
        element->next = head;
        sq->sensors[act_sensor_mod][act_sensor_no] = element;
    } else {
        // iterate until in right position
        struct sensor_queue_entry *curr = head;
        while (curr->next != NULL &&
            curr->next->data.expected_time < element->data.expected_time) {
            curr = curr->next;
        }

        element->next = curr->next;
        curr->next = element;
    }
}

// add a process to the waiting list
void sensor_queue_add_waiting_tid(sensor_queue *sq, uint16_t sensor_mod,
    uint16_t sensor_no, sensor_data *data) {
    int act_sensor_mod = sensor_mod - 1;
    int act_sensor_no = sensor_no - 1;
    uassert(0 <= act_sensor_mod && act_sensor_mod < N_SENSOR_MODULES);
    uassert(0 <= act_sensor_no && act_sensor_no < N_SENSORS);

    if (sq->freelist == NULL) {
        // TODO: error message
        upanic("Cannot enqueue waiting task to sensor queue\r\n");
    } else {
        // get new element
        struct sensor_queue_entry *element = sq->freelist;
        sq->freelist = sq->freelist->next;

        // copy data
        memcpy(&element->data, data, sizeof(sensor_data));

        sensor_queue_insert(sq, act_sensor_mod, act_sensor_no, element);

        // if position -> add to timeout queue
        if (data->pos_rqst) {
            int train_hash = trn_hash(data->trn);
            sq->sensor_timeout[train_hash] = element;
        }
    }
}

int
sensor_queue_get_waiting_tid(sensor_queue *sq, uint16_t sensor_mod,
    uint16_t sensor_no, uint32_t activation_time, sensor_data *data) {
    int act_sensor_mod = sensor_mod - 1;
    int act_sensor_no = sensor_no - 1;
    uassert(0 <= act_sensor_mod && act_sensor_mod < N_SENSOR_MODULES);
    uassert(0 <= act_sensor_no && act_sensor_no < N_SENSORS);

    struct sensor_queue_entry *head = sq->sensors[act_sensor_mod][act_sensor_no];

    if (head == NULL) {
        return SENSOR_QUEUE_DONE;
    }

    if (head->data.expected_time > activation_time + TIMEOUT_TICKS) {
        return SENSOR_QUEUE_EARLY;
    }

    // pop off queue
    memcpy(data, &head->data, sizeof(sensor_data));
    sq->sensors[act_sensor_mod][act_sensor_no] = head->next;

    // reset the queue entry and add to free list
    memset(head, 0, sizeof(sensor_queue_entry));
    head->next = sq->freelist;
    sq->freelist = head;

    ULOG("[sensor get] activation: %u expected: %u timeout: %u\r\n",
        activation_time, data->expected_time, TIMEOUT_TICKS);

    if (data->expected_time == TIME_NONE)
        return SENSOR_QUEUE_FOUND;
    
    if (data->pos_rqst) {
        // deque from timeout struct
        int train_hash = trn_hash(data->trn);
        if (sq->sensor_timeout[train_hash] == head) {
            sq->sensor_timeout[train_hash] = NULL;
        }
    }

    return (activation_time - TIMEOUT_TICKS <= data->expected_time
        && data->expected_time <= activation_time + TIMEOUT_TICKS)
        ? SENSOR_QUEUE_FOUND : SENSOR_QUEUE_TIMEOUT;
}

int sensor_queue_check_timeout(sensor_queue *sq, int8_t train_hash, uint32_t activation_time) {
    // check if that trainNo is empty    
    if (sq->sensor_timeout[train_hash] == NULL) {
        return SENSOR_QUEUE_DONE;
    } 

    if (sq->sensor_timeout[train_hash]->data.expected_time + TIMEOUT_TICKS < activation_time) {
        // Case that we actually have a timeout
        // set it to NULL -> we only want one timeout activation
        return SENSOR_QUEUE_TIMEOUT;
    } else {
        return SENSOR_QUEUE_DONE;
    }
}


void sensor_queue_adjust_waiting_tid(sensor_queue *sq, uint16_t sensor_mod,
    uint16_t sensor_no, uint16_t tid, int64_t expected_time) {
    int act_sensor_mod = sensor_mod - 1;
    int act_sensor_no = sensor_no - 1;
    uassert(0 <= act_sensor_mod && act_sensor_mod < N_SENSOR_MODULES);
    uassert(0 <= act_sensor_no && act_sensor_no < N_SENSORS);

    struct sensor_queue_entry *curr = sq->sensors[act_sensor_mod][act_sensor_no];
    struct sensor_queue_entry *prev = NULL;

    while (curr != NULL) {
        if (curr->data.tid == tid) {
            // pop off queue
            if (prev == NULL) {
                // at head
                sq->sensors[act_sensor_mod][act_sensor_no] = curr->next;
            } else {
                prev->next = curr->next;
            }

            curr->next = NULL;
            break;
        }

        prev = curr;
        curr = curr->next;
    }

    if (curr == NULL) return; // no found

    // reinsert in new position
    curr->data.expected_time = expected_time;
    sensor_queue_insert(sq, act_sensor_mod, act_sensor_no, curr);
}

void sensor_queue_free_train(sensor_queue *sq, uint16_t trn) {
    for (uint8_t i = 0; i < N_SENSOR_MODULES; ++i) {
        for (uint8_t j = 0; j < N_SENSORS; ++j) {
            struct sensor_queue_entry *prev = NULL;
            struct sensor_queue_entry *curr = sq->sensors[i][j];

            while (curr != NULL) {
                if (curr->data.trn == trn) {
                    // remove from queue
                    if (prev == NULL) {
                        sq->sensors[i][j] = curr->next;
                    } else {
                        prev->next = curr->next;
                    }

                    // reset & add to freelist
                    memset(curr, 0, sizeof(sensor_queue_entry));

                    curr->next = sq->freelist;
                    sq->freelist = curr;
                } else {
                    prev = curr; // if removing, prev node stays same
                }

                curr = curr->next;
            }
        }
    }
}
