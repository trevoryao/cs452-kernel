#include "sensor-queue.h"

#include "uassert.h"
#include "util.h"

#include "speed.h"
#include "speed-data.h"
#include "track-data.h"

extern speed_data spd_data;

void sensor_data_init(sensor_data *data, uint8_t trn) {
    data->trn = trn;
    data->first_activation = TIME_NONE;
}

void sensor_queue_init(sensor_queue *sq, speed_t *spd) {
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

    sq->spd = spd;
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

void sensor_queue_wait(sensor_queue *sq, track_node *node, uint8_t trn) {
    // verify is a sensor
    uassert(node->num < NUM_SEN_PER_MOD * NUM_MOD);

    sensor_queue_entry *ele = sensor_queue_alloc_entry(sq);
    sensor_data_init(&ele->data, trn);

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

int64_t sensor_queue_update(sensor_queue *sq, track_node *node,
    uint32_t activation_time) {
    uassert(node->num < NUM_SEN_PER_MOD * NUM_MOD);
    sensor_queue_entry *head = sq->sensors_front[node->num];

    if (head == NULL) {
        // drop, either spurious or rest of train
        return -1;
    }

    ULOG("[sensor-queue] got meaningful activation for %s\r\n", node->name);

    if (head->data.first_activation == TIME_NONE) {
        head->data.first_activation = activation_time;
        ULOG("[sensor-queue] first activation %s\r\n", node->name);
        return -1;
    }

    // check if we have timed out, or still the same train
    uint32_t timeout = get_time_from_velocity(&spd_data, head->data.trn, TRN_LEN_MM, speed_display_get(sq->spd, head->data.trn));

    ULOG("[sensor-queue] checking timeout %d ticks (speed %d)\r\n",
        timeout, speed_display_get(sq->spd, head->data.trn));

    uint32_t time_from_first = activation_time - head->data.first_activation;

    ULOG("[sensor-queue] offset cmp: %d\r\n", time_from_first);

    if (time_from_first >= timeout) {
        // timeout -- must be the second train
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

        ULOG("[sensor-queue] new train\r\n");
        return time_from_first;
    } else {
        ULOG("[sensor-queue] same train, ignoring\r\n");
        return -1; // same train, don't care
    }
}
