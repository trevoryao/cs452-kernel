#include "routing.h"

#include "priority-queue.h"
#include "uassert.h"
#include "util.h"

#include "controller-consts.h"
#include "speed-data.h"
#include "track-data.h"
#include "track-server.h"
#include "track-segment-locking.h"

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
    deque_push_back(&raq->q, (int)action->action.total);
    deque_push_back(&raq->q, (int)action->info.delay_ticks);
}

void routing_action_queue_push_front(routing_action_queue *raq, routing_action *action) {
    deque_push_front(&raq->q, (int)action->info.delay_ticks);
    deque_push_front(&raq->q, (int)action->action.total);
    deque_push_front(&raq->q, action->action_type);
    deque_push_front(&raq->q, (int)action->sensor_num);
}

void routing_action_queue_pop_front(routing_action_queue *raq, routing_action *action) {
    if (action) {
        action->sensor_num = (int16_t)deque_pop_front(&raq->q);
        action->action_type = deque_pop_front(&raq->q);
        action->action.total = (uint16_t)deque_pop_front(&raq->q);
        action->info.delay_ticks = (uint32_t)deque_pop_front(&raq->q);
    } else {
        deque_pop_front(&raq->q);
        deque_pop_front(&raq->q);
        deque_pop_front(&raq->q);
        deque_pop_front(&raq->q);
    }
}

void routing_action_queue_pop_back(routing_action_queue *raq, routing_action *action) {
    if (action) {
        action->info.delay_ticks = (uint32_t)deque_pop_back(&raq->q);
        action->action.total = (uint16_t)deque_pop_back(&raq->q);
        action->action_type = deque_pop_back(&raq->q);
        action->sensor_num = (int16_t)deque_pop_back(&raq->q);
    } else {
        deque_pop_back(&raq->q);
        deque_pop_back(&raq->q);
        deque_pop_back(&raq->q);
        deque_pop_back(&raq->q);
    }
}

void routing_action_queue_front(routing_action_queue *raq, routing_action *action) {
    deque_itr itr = deque_begin(&raq->q);
    action->sensor_num = (int16_t)deque_itr_get(&raq->q, itr);

    itr = deque_itr_next(itr);
    action->action_type = deque_itr_get(&raq->q, itr);

    itr = deque_itr_next(itr);
    action->action.total = (uint16_t)deque_itr_get(&raq->q, itr);
    itr = deque_itr_next(itr);
    action->info.delay_ticks = (uint32_t)deque_itr_get(&raq->q, itr);
}

void routing_actions_init(routing_actions *actions) {
    routing_action_queue_init(&actions->path);
    routing_action_queue_init(&actions->speed_changes);
    deque_init(&actions->segments, 3);
    actions->state = NORMAL_SEGMENT;
}

#define HASH(node) ((node) - track)
#define DIST_TRAVELLED(h1, h2) ((dist[(h2)] - dist[(h1)]) * MM_TO_UM)

#define SW_DELAY_TICKS 75
#define SPD_REACH_WINDOW 10
#define IN_USE_PENALTY 2 // double length

static inline uint8_t calculate_nbhd_size(node_type type) {
    switch (type) {
        case NODE_BRANCH: return 2;
        case NODE_EXIT: return 0;
        default: return 1;
    }
}

static inline int32_t calculate_stopping_delay(uint16_t trn, int32_t stopping_distance,
    int32_t sensor_distance, enum SPEEDS start_spd) {

    int64_t nominator = ((sensor_distance - stopping_distance) * 1000);
    int64_t denominator = get_velocity(&spd_data, trn, start_spd);
    int64_t result = nominator / denominator;
    return (int32_t)(result / 10);
}

static const int32_t MIN_ROLL_DIST = 100 * MM_TO_UM; // um (100mm)

// main method for path finding
// assumes train server has found track node num in track[]
// for start/finish nodes
static void
plan_direct_route(track_node *start_node, track_node *end_node,
    int16_t offset, int16_t trn, enum SPEEDS start_spd,
    enum SPEEDS target_spd, bool include_first_node,
    int track_server_tid, routing_actions *route) {

    int32_t dist[TRACK_MAX]; // actual dist (speed calculations)
    int32_t weighted_dist[TRACK_MAX]; // modified path weight dependent of locked state, etc
    track_node *prev[TRACK_MAX];

    priority_queue pq;

    dist[HASH(start_node)] = 0;
    weighted_dist[HASH(start_node)] = 0;
    prev[HASH(start_node)] = NULL;

    for (int16_t i = 0; i < TRACK_MAX; ++i) {
        if (i != HASH(start_node)) {
            dist[i] = INT32_MAX;
            weighted_dist[i] = INT32_MAX;
            prev[i] = NULL;
        }

        // just add into heap directly, init will heapify
        pq.heap[i].value = &track[i];
        pq.heap[i].priority = weighted_dist[i];
    }

    // cache segment calls
    #define SEGMENT_CACHE_NONE 0
    #define SEGMENT_CACHE_UNLOCKED 1
    #define SEGMENT_CACHE_LOCKED 2
    int8_t segment_cache[N_SEGMENTS] = {0};

    priority_queue_init(&pq, TRACK_MAX); // heapifies

    while (!priority_queue_empty(&pq)) {
        track_node *node = priority_queue_pop_min(&pq);
        uassert(node);
        if (node == end_node || node->reverse == end_node) {
            ULOG("found route\r\n");
            end_node = node;
            break; // reached!
        }

        if (dist[HASH(node)] == INT32_MAX) { // can't reach?
            ULOG("node was unreachable\r\n");
            route->state = ERR_NO_ROUTE;
            return;
        }

        uint8_t nbhd_size = calculate_nbhd_size(node->type);

        for (uint8_t i = 0; i < nbhd_size; ++i) {
            track_node *nbr = node->edge[i].dest;
            uassert(nbr);

            // get segment locked state for fixed penalty
            if (segment_cache[node->segmentId] == SEGMENT_CACHE_NONE) {
                segment_cache[node->segmentId] =
                    track_server_segment_is_locked(track_server_tid, node->segmentId, trn)
                    ? SEGMENT_CACHE_LOCKED : SEGMENT_CACHE_UNLOCKED;
            }

            int mult = (segment_cache[node->segmentId] == SEGMENT_CACHE_LOCKED)
                ? IN_USE_PENALTY : 1;

            int32_t alt_path_dist = dist[HASH(node)] + node->edge[i].dist;
            int32_t alt_path_weighted_dist = dist[HASH(node)] + (mult * node->edge[i].dist);

            if (alt_path_weighted_dist < weighted_dist[HASH(nbr)]) {
                weighted_dist[HASH(nbr)] = alt_path_weighted_dist;
                dist[HASH(nbr)] = alt_path_dist;
                prev[HASH(nbr)] = node;
                uassert(priority_queue_decrease(&pq, nbr, alt_path_weighted_dist));
            }
        }
    }

    // backtrack to find route
    int32_t stopping_dist = get_stopping_distance(&spd_data, trn, target_spd);

    // TODO: check if short move

    // queueing branch nodes
    deque branches;
    deque_init(&branches, 5);

    routing_action action; // used for pushing back actions

    track_node *next_sensor = NULL;
    track_node *next = NULL;
    track_node *node = end_node;

    track_node *end = include_first_node ? NULL : start_node;

    // find stopping node and determine if we need to stop on this iteration
    track_node *stopping_node = NULL;
    while (node != end) { // only in an emergency, should not happen
        if (deque_empty(&route->segments) || deque_front(&route->segments) != node->reverse->segmentId) {
            // track which segment we are currently in
            if (stopping_node != NULL) break; // segment change point
            deque_push_front(&route->segments, node->reverse->segmentId);
        }

        if (node->type == NODE_BRANCH) {
            int8_t branch_dir;
            if (next) { // only if we are stopping on a branch node
                branch_dir = (node->edge[DIR_CURVED].dest == next) ? DIR_CURVED : DIR_STRAIGHT;
            } else {
                branch_dir = DIR_STRAIGHT; // default straight
            }

            deque_push_back(&branches, HASH(node)); // node
            deque_push_back(&branches, branch_dir); // branch_dir
        }

        if (node->type != NODE_SENSOR) { // go to prev node
            next = node;
            node = prev[HASH(next)];
            continue;
        }

        int32_t dist_travelled = DIST_TRAVELLED(HASH(node),
            HASH(end_node)) + (offset * MM_TO_UM);

        if (stopping_node == NULL && dist_travelled > stopping_dist) {
            if (node->segmentId == start_node->segmentId ||
                node->reverse->segmentId == start_node->segmentId) {
                // need to stop?
                action.sensor_num = node->num;
                action.action_type = SPD_CHANGE;
                action.action.spd = SPD_STP;
                action.info.delay_ticks = calculate_stopping_delay(trn, stopping_dist, dist_travelled, target_spd);
                routing_action_queue_push_front(&route->speed_changes,
                    &action);

                stopping_node = node; // save for next loop

                // want to go to the end of the segment
            } else {
                // clear any gathered data
                routing_action_queue_reset(&route->path);
                deque_reset(&route->segments);
                deque_reset(&branches);

                break; // break early
            }
        }

        if (next_sensor != NULL) {
            // add distance information
            action.sensor_num = node->num;
            action.action_type = SENSOR;
            action.action.total = 0;
            action.info.dist = DIST_TRAVELLED(HASH(node), HASH(next_sensor)) / MM_TO_UM;
            routing_action_queue_push_front(&route->path, &action);
        }

        next = node;
        next_sensor = next;
        node = prev[HASH(next)];
    }

    // throw any switches needed during stopping
    if (stopping_node != NULL) {
        while (!deque_empty(&branches)) {
            track_node *branch_node = &track[deque_pop_front(&branches)];
            int8_t branch_dir = deque_pop_front(&branches);

            action.sensor_num = stopping_node->num;
            action.action_type = SWITCH;
            action.action.sw.num = branch_node->num;
            action.action.sw.dir = (branch_dir == DIR_CURVED) ? CRV : STRT;
            action.info.delay_ticks = 0;
            routing_action_queue_push_front(&route->path, &action);
        }
    }

    // calculate from start_node to end of start node sector
    while (node->reverse->segmentId != start_node->segmentId) {
        if (deque_empty(&route->segments) || deque_front(&route->segments) != node->reverse->segmentId) {
            // only save last seen segment while iterating
            if (!deque_empty(&route->segments))
                deque_pop_front(&route->segments);
                // track which segment we are currently in
            deque_push_front(&route->segments, node->reverse->segmentId);
        }

        // go to our desired segment
        if (node->type == NODE_SENSOR)
            next_sensor = node;

        next = node;
        node = prev[HASH(next)];
    }

    track_node *sector_end = (end_node != NULL) ? node : NULL;

    // in target sector
    while (node != end) {
        if (node->type == NODE_BRANCH) {
            int8_t branch_dir;
            if (next) { // only if we are stopping on a branch node
                branch_dir = (node->edge[DIR_CURVED].dest == next) ? DIR_CURVED : DIR_STRAIGHT;
            } else {
                branch_dir = DIR_STRAIGHT; // default straight
            }

            deque_push_back(&branches, HASH(node)); // node
            deque_push_back(&branches, branch_dir); // branch_dir
        }

        if (node->type != NODE_SENSOR) { // go to prev node
            next = node;
            node = prev[HASH(next)];
            continue;
        }

        if (next_sensor != NULL) {
            // add distance information
            action.sensor_num = node->num;
            action.action_type = SENSOR;
            action.action.total = 0;
            action.info.dist = DIST_TRAVELLED(HASH(node), HASH(next_sensor)) / MM_TO_UM;
            routing_action_queue_push_front(&route->path, &action);
        }

        next = node;
        next_sensor = next;
        node = prev[HASH(next)];
    }

    // calculate next decision point
    if (sector_end != NULL) {
        int32_t decision_dist = DIST_TRAVELLED(HASH(start_node),
            HASH(sector_end)) - (100 * MM_TO_UM);

        uint32_t decision_delay_time;

        if (start_spd == target_spd) {
            // assume constant
            decision_delay_time = get_time_from_velocity_um(&spd_data, trn,
                decision_dist, target_spd);
        } else {
            // do we reach constant spd before hand?
            int32_t accel_dist = get_distance_from_acceleration(&spd_data,
                trn, start_spd, target_spd);

            if (accel_dist < decision_dist) {
                // add accel and const times
                decision_delay_time =
                    get_time_from_acceleration(&spd_data, trn,
                        start_spd, target_spd) +
                    get_time_from_velocity_um(&spd_data, trn,
                        decision_dist - accel_dist, target_spd);
            } else {
                // estimate at full const
                decision_delay_time =
                    estimate_initial_time_acceleration(&spd_data, trn,
                        start_spd, target_spd, decision_dist);
            }
        }

        // add decision pt to queue
        action.sensor_num = start_node->num;
        action.action_type = DECISION_PT;
        action.info.delay_ticks = decision_delay_time;
        action.action.total = 0; // memset
        routing_action_queue_push_front(&route->speed_changes, &action);
    }

    // calculate acceleration data if needed
    if (start_spd != target_spd) {
        int32_t start_time = get_time_from_acceleration(&spd_data, trn, start_spd, target_spd);

        action.sensor_num = SENSOR_NONE;
        action.action_type = SPD_REACHED;
        action.action.spd = target_spd;
        action.info.delay_ticks = start_time + SPD_REACH_WINDOW;
        routing_action_queue_push_front(&route->speed_changes, &action);

        action.sensor_num = SENSOR_NONE;
        action.action_type = SPD_CHANGE;
        action.action.spd = target_spd;
        action.info.delay_ticks = 0;
        routing_action_queue_push_front(&route->speed_changes, &action);
    }

    route->state = (stopping_node != NULL) ?
        FINAL_SEGMENT : NORMAL_SEGMENT;
}

void plan_in_progress_route(track_node *start_node, track_node *end_node,
    int16_t offset, int16_t trn, enum SPEEDS spd, int track_server_tid,
    routing_actions *route) {
    plan_direct_route(start_node, end_node, offset, trn, spd, spd, true, track_server_tid, route);
}

void plan_stopped_route(track_node *start_node, track_node *end_node,
    int16_t offset, int16_t trn, enum SPEEDS spd, int track_server_tid,
    routing_actions *fwd_route, routing_actions *rv_route) {
    plan_direct_route(start_node, end_node, offset, trn, SPD_STP, spd, true, track_server_tid, fwd_route);
    plan_direct_route(start_node->reverse, end_node, offset, trn, SPD_STP, spd, false, track_server_tid, rv_route);
}
