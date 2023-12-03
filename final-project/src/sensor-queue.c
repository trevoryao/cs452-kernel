#include "sensor-queue.h"

#include "uassert.h"
#include "util.h"

#include "snake.h"
#include "speed.h"
#include "speed-data.h"
#include "track-data.h"

extern speed_data spd_data;

void sensor_data_init(sensor_data *data, uint8_t snake_idx, bool single_train) {
    data->snake_idx = snake_idx;
    data->first_activation = TIME_NONE;
    data->single_train = (int8_t)single_train;
}

void sensor_queue_init(sensor_queue *sq, snake *snake) {
    memset(sq->storage, 0, sizeof(sensor_queue_entry) *
        MAX_WAITING_PROCESSES);

    memset(sq->sensors_front, 0, sizeof(sensor_queue_entry) *
        (NUM_SEN_PER_MOD * NUM_MOD));

    memset(sq->sensors_front, 0, sizeof(sensor_queue_entry) *
        (NUM_SEN_PER_MOD * NUM_MOD));

    sq->freelist = NULL;

    for (int i = 0; i < MAX_WAITING_PROCESSES; i++) {
        sq->storage[i].next = sq->freelist;
        sq->freelist = &sq->storage[i];
    }

    sq->snake = snake;
}

bool sensor_queue_is_waiting(sensor_queue *sq, track_node *node) {
    uassert(node->num < NUM_SEN_PER_MOD * NUM_MOD);
    return sq->sensors_front[node->num] != NULL;
}

static sensor_queue_entry *
sensor_queue_alloc_entry(sensor_queue *sq) {
    if (sq->freelist == NULL) {
        upanic("sensor queue out of memory\r\n");
    }

    sensor_queue_entry *ele = sq->freelist;
    sq->freelist = sq->freelist->next;
    ele->next = NULL;

    return ele;
}

static void
sensor_queue_free_entry(sensor_queue *sq, sensor_queue_entry *ele) {
    ele->next = sq->freelist;
    sq->freelist = ele;
}

void sensor_queue_wait(sensor_queue *sq, track_node *node,
    uint8_t snake_idx, bool single_train) {
    // verify is a sensor
    uassert(node->num < NUM_SEN_PER_MOD * NUM_MOD);

    sensor_queue_entry *ele = sensor_queue_alloc_entry(sq);
    sensor_data_init(&ele->data, snake_idx, single_train);

    // insert into back of queue
    if (sq->sensors_back[node->num] == NULL) {
        // empty queue
        sq->sensors_front[node->num] = ele;
        sq->sensors_back[node->num] = ele;
    } else {
        sq->sensors_back[node->num]->next = ele;
        sq->sensors_back[node->num] = ele;
    }
}

static void
sensor_queue_pop(sensor_queue *sq, track_node *node, uint32_t activation_time) {
    sensor_queue_entry *head = sq->sensors_front[node->num]; // unchecked

    // pop head off and possibly update next value
    if (head->next != NULL) {
        sq->sensors_front[node->num] = head->next;
        // set next activation time (for following)
        head->next->data.first_activation = activation_time;
    } else {
        // queue is empty now
        sq->sensors_front[node->num] = NULL;
        sq->sensors_back[node->num] = NULL;
    }

    sensor_queue_free_entry(sq, head);
}

int64_t sensor_queue_update(sensor_queue *sq, track_node *node,
    uint32_t activation_time, uint8_t *snake_idx) {
    uassert(node->num < NUM_SEN_PER_MOD * NUM_MOD);
    sensor_queue_entry *head = sq->sensors_front[node->num];

    if (head == NULL) {
        // drop, either spurious or rest of train
        return ERR_SPURIOUS;
    }

    uassert(snake_idx != NULL);
    // return to caller
    *snake_idx = head->data.snake_idx;

    if (head->data.first_activation == TIME_NONE) {
        if (head->data.single_train) {
            sensor_queue_pop(sq, node, activation_time);
        } else {
            head->data.first_activation = activation_time;
        }
        return FIRST_ACTIVATION;
    }

    uint8_t trn = sq->snake->trns[head->data.snake_idx].trn;

    // check if we have timed out, or still the same train
    uint32_t timeout = get_time_from_velocity(&spd_data, trn, TRN_TIMEOUT_DIST, speed_display_get(&sq->snake->spd_t, trn));

    uint32_t time_from_first = activation_time - head->data.first_activation;

    if (time_from_first >= timeout) {
        // timeout -- must be the second train
        sensor_queue_pop(sq, node, activation_time);
        return time_from_first;
    } else {
        return SAME_TRAIN; // same train, don't care
    }
}
