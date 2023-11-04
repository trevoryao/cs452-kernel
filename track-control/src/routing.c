#include "routing.h"

#include "priority-queue.h"
#include "uassert.h"
#include "util.h"

#include "controller-consts.h"
#include "speed-data.h"
#include "track-data.h"

extern speed_data spd_data;
extern track_node track[];

void routing_action_queue_init(routing_action_queue *raq) {
    deque_init(&raq->q, 10);
}

void routing_action_queue_reset(routing_action_queue *raq) {
    deque_reset(&raq->q);
}

bool routing_action_queue_empty(routing_action_queue *raq) {
    return deque_empty(&raq->q);
}

bool routing_action_queue_full(routing_action_queue *raq) {
    return deque_full(&raq->q);
}

uint16_t routing_action_queue_size(routing_action_queue *raq) {
    return (deque_size(&raq->q) >> 2); // div 4
}

void routing_action_queue_push_back(routing_action_queue *raq, routing_action *action) {
    deque_push_back(&raq->q, (int)action->sensor_num);
    deque_push_back(&raq->q, action->action_type);
    deque_push_back(&raq->q, (int)action->action.sw_num); // don't care since same size
    deque_push_back(&raq->q, (int)action->delay_ticks);
}

void routing_action_queue_push_front(routing_action_queue *raq, routing_action *action) {
    deque_push_front(&raq->q, (int)action->delay_ticks);
    deque_push_front(&raq->q, (int)action->action.sw_num); // don't care since same size
    deque_push_front(&raq->q, action->action_type);
    deque_push_front(&raq->q, (int)action->sensor_num);
}

void routing_action_queue_pop_front(routing_action_queue *raq, routing_action *action) {
    action->sensor_num = (int16_t)deque_pop_front(&raq->q);
    action->action_type = deque_pop_front(&raq->q);
    action->action.sw_num = (uint8_t)deque_pop_front(&raq->q); // don't care since same size
    action->delay_ticks = (uint32_t)deque_pop_front(&raq->q);
}

void routing_action_queue_pop_back(routing_action_queue *raq, routing_action *action) {
    action->delay_ticks = (uint32_t)deque_pop_back(&raq->q);
    action->action.sw_num = (uint8_t)deque_pop_back(&raq->q); // don't care since same size
    action->action_type = deque_pop_back(&raq->q);
    action->sensor_num = (int16_t)deque_pop_back(&raq->q);
}

void routing_action_queue_front(routing_action_queue *raq, routing_action *action) {
    deque_itr itr = deque_begin(&raq->q);

    action->sensor_num = (int16_t)deque_itr_get(&raq->q, itr);
    action->action_type = deque_itr_get(&raq->q, deque_itr_next(itr));
    action->action.sw_num = (uint8_t)deque_itr_get(&raq->q, deque_itr_next(itr)); // don't care since same size
    action->delay_ticks = (uint32_t)deque_itr_get(&raq->q, deque_itr_next(itr));
}

#define HASH(node) (node) - track

static inline int32_t calculate_stopping_delay(uint16_t trn, int32_t stopping_distance,
    int32_t sensor_distance, enum SPEEDS start_spd) {
    return ((sensor_distance - stopping_distance) * 100) / get_velocity(&spd_data, trn, start_spd);
}

void plan_route(track_node *start_node, track_node *end_node,
    int16_t offset, int16_t trn, enum SPEEDS start_spd,
    enum SPEEDS target_spd,
    routing_action_queue *path, routing_action_queue *speed_changes) {
    // dijkstra using indices of the track[]
    int32_t dist[TRACK_MAX];
    track_node *prev[TRACK_MAX];

    priority_queue pq;

    dist[HASH(start_node)] = 0;
    prev[HASH(start_node)] = NULL;

    for (int16_t i = 0; i < TRACK_MAX; ++i) {
        if (i != HASH(start_node)) {
            dist[i] = INT32_MAX;
            prev[i] = NULL;
        }

        // just add into heap directly, init will heapify
        pq.heap[i].value = track + i;
        pq.heap[i].priority = dist[i];
    }

    priority_queue_init(&pq); // heapifies

    while (!priority_queue_empty(&pq)) {
        track_node *node = priority_queue_pop_min(&pq);
        if (node == end_node)
            break; // reached!

        if (dist[HASH(node)] == INT32_MAX) // can't reach?
            return;

        int nbhd_size = ((node->type == NODE_BRANCH) ? DIR_CURVED : DIR_AHEAD) + 1;

        for (int i = 0; i < nbhd_size; ++i) {
            track_node *nbr = node->edge[i].dest;
            int32_t alt = dist[HASH(node)] + node->edge[i].dist;

            if (alt < dist[HASH(nbr)]) {
                dist[HASH(nbr)] = alt;
                prev[HASH(nbr)] = node;
            }
        }
    }

    // backtrack to find route
    int32_t route_dist = offset * MM_TO_UM;

    // to go from 7 -> 0
    int32_t stopping_dist = get_stopping_distance(&spd_data, trn);
    // to go from 9 -> 7
    int32_t stopping_prep_dist = (target_spd == SPD_LO) ? -1 :
        get_distance_from_acceleration(&spd_data, trn, target_spd, SPD_LO);

    int8_t sensors_before_branch = 0;
    track_node *branch_node = NULL;

    routing_action action; // used for pushing back actions

    track_node *next = end_node;
    track_node *next_sensor = end_node; // assume enough space to have at least one sensor
    track_node *node = prev[HASH(end_node)];
    while (node != NULL) {
        if (node->type == NODE_BRANCH) {
            branch_node = node;

            route_dist += node->edge[(node->edge[DIR_CURVED].dest == next) ? DIR_CURVED : DIR_STRAIGHT].dist * MM_TO_UM;
        } else {
            route_dist += node->edge[DIR_AHEAD].dist * MM_TO_UM;
        }

        if (node->type != NODE_SENSOR) { // go to prev node
            next = node;
            node = prev[HASH(next)];
            continue;
        }

        if (stopping_dist != -1 && route_dist > stopping_dist) {
            // no delay if we will do any deaccel to speed 7
            action.sensor_num = (stopping_prep_dist == -1) ? node->num : SENSOR_NONE;
            action.action_type = SPD_CHANGE;
            action.action.spd = SPD_STP;
            action.delay_ticks = (stopping_prep_dist == -1) ?
                calculate_stopping_delay(trn, stopping_dist, route_dist, target_spd) : 0;

            routing_action_queue_push_front(speed_changes, &action);

            stopping_dist = -1; // don't need to check anymore
        }

        if (stopping_prep_dist != -1 && route_dist > stopping_prep_dist + stopping_dist) {
            // if deaccel always need to delay
            action.sensor_num = node->num;
            action.action_type = SPD_CHANGE;
            action.action.spd = SPD_LO;
            action.delay_ticks = calculate_stopping_delay(trn, stopping_prep_dist + stopping_dist, route_dist, target_spd);

            routing_action_queue_push_front(speed_changes, &action);

            stopping_prep_dist = -1; // stop checking
        }

        if (branch_node) {
            if (++sensors_before_branch == 2) {
                // add switch throw
                action.sensor_num = node->num;
                action.action_type = SWITCH;
                action.action.sw_num = branch_node->num;
                action.delay_ticks = 0;

                routing_action_queue_push_front(path, &action);

                branch_node = NULL;
                sensors_before_branch = 0;
            }
        }

        next_sensor = node;
        next = node;
        node = prev[HASH(next)];
    }

    // add start up acceleration data
    // next holds first node
    // to go from 0 -> 7 -> spd
    int32_t initial_start_time = get_time_from_acceleration(&spd_data, trn, SPD_STP, SPD_LO);
    int32_t secondary_start_time = get_time_from_acceleration(&spd_data, trn, SPD_LO, target_spd);

    if (secondary_start_time == 0) { // no extra acceleration to target spd? (target_spd == SPD_LO)
        action.sensor_num = SENSOR_NONE;
        action.action_type = SPD_REACHED;
        action.action.sw_num = 0;
        action.delay_ticks = initial_start_time;
        routing_action_queue_push_front(speed_changes, &action);
    } else {
        action.sensor_num = SENSOR_NONE;
        action.action_type = SPD_REACHED;
        action.action.sw_num = 0;
        action.delay_ticks = secondary_start_time;
        routing_action_queue_push_front(speed_changes, &action);

        action.sensor_num = start_node->num;
        action.action_type = SPD_CHANGE;
        action.action.spd = target_spd;
        action.delay_ticks = initial_start_time;
        routing_action_queue_push_front(speed_changes, &action);
    }

    action.sensor_num = start_node->num;
    action.action_type = SPD_CHANGE;
    action.action.spd = SPD_LO;
    action.delay_ticks = 0;
    routing_action_queue_push_front(speed_changes, &action);
}
