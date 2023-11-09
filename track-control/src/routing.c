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

#define HASH(node) ((node) - track)
#define DIST_TRAVELLED(h1, h2) ((dist[(h2)] - dist[(h1)]) * MM_TO_UM)

#define SW_DELAY_TICKS 300

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

static const int32_t MIN_ROLL_DIST = 500 * MM_TO_UM; // um (500)
// main method for path finding
// assumes train server has found track node num in track[]
// for start/finish nodes
// return codes:
#define ERR_NO_ROUTE -1
#define ERR_TOO_SHORT -2
static int32_t plan_direct_route(track_node *start_node, track_node *end_node,
    int16_t offset, int16_t trn, enum SPEEDS start_spd,
    enum SPEEDS target_spd, bool include_first_node,
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
        pq.heap[i].value = &track[i];
        pq.heap[i].priority = dist[i];
    }

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
            return ERR_NO_ROUTE;
        }

        uint8_t nbhd_size = calculate_nbhd_size(node->type);

        for (uint8_t i = 0; i < nbhd_size; ++i) {
            track_node *nbr = node->edge[i].dest;
            uassert(nbr);
            int32_t alt = dist[HASH(node)] + node->edge[i].dist;

            if (alt < dist[HASH(nbr)]) {
                dist[HASH(nbr)] = alt;
                prev[HASH(nbr)] = node;
                uassert(priority_queue_decrease(&pq, nbr, alt));
            }
        }
    }

    // backtrack to find route
    int32_t stopping_dist = get_stopping_distance(&spd_data, trn, target_spd);

    if (start_spd != target_spd) {
        // can we actually get there?
        int32_t startup_dist =
            get_distance_from_acceleration(&spd_data, trn, start_spd, SPD_LO) +
            get_distance_from_acceleration(&spd_data, trn, SPD_LO, target_spd);
        uint64_t min_path_dist = startup_dist + MIN_ROLL_DIST + stopping_dist;

        if ((uint32_t)(dist[HASH(end_node)] * MM_TO_UM) < min_path_dist) {
            ULOG("error too short\r\n");
            return ERR_TOO_SHORT;
        }
    }

    int32_t min_dist_before_branch = get_distance_from_velocity(&spd_data, trn, SW_DELAY_TICKS, target_spd);

    // queueing branch nodes
    deque branches;
    deque_init(&branches, 5);

    routing_action action; // used for pushing back actions

    track_node *next_sensor = NULL;
    track_node *next = NULL;
    track_node *node = end_node;

    track_node *end = include_first_node ? NULL : start_node;

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

        int32_t dist_travelled = DIST_TRAVELLED(HASH(node),
            HASH(end_node)) + (offset * MM_TO_UM);

        while (!deque_empty(&branches) &&
            DIST_TRAVELLED(HASH(node), deque_front(&branches)) > min_dist_before_branch) {
            track_node *branch_node = &track[deque_pop_front(&branches)];
            int8_t branch_dir = deque_pop_front(&branches);

            action.sensor_num = node->num;
            action.action_type = SWITCH;
            action.action.sw.num = branch_node->num;
            action.action.sw.dir = (branch_dir == DIR_CURVED) ? CRV : STRT;
            action.info.delay_ticks = 0;
            routing_action_queue_push_front(path, &action);
        }

        if (stopping_dist != -1 && dist_travelled > stopping_dist) {
            // no delay if we will do any deaccel to speed 7
            action.sensor_num = node->num;
            action.action_type = SPD_CHANGE;
            action.action.spd = SPD_STP;
            action.info.delay_ticks = calculate_stopping_delay(trn, stopping_dist, dist_travelled, target_spd);

            routing_action_queue_push_front(speed_changes, &action);
            stopping_dist = -1; // only check on the first possibly place
        }

        if (next_sensor != NULL) {
            // add distance information
            action.sensor_num = node->num;
            action.action_type = SENSOR;
            action.action.total = 0;
            action.info.dist = DIST_TRAVELLED(HASH(node), HASH(next_sensor)) / MM_TO_UM;
            routing_action_queue_push_front(path, &action);
        }

        next = node;
        next_sensor = next;
        node = prev[HASH(next)];
    }

    // throw any leftover switches immediately without any time delay
    while (!deque_empty(&branches)) {
        track_node *branch_node = &track[deque_pop_front(&branches)];
        int8_t branch_dir = deque_pop_front(&branches);

        action.sensor_num = SENSOR_NONE;
        action.action_type = SWITCH;
        action.action.sw.num = branch_node->num;
        action.action.sw.dir = (branch_dir == DIR_CURVED) ? CRV : STRT;
        action.info.delay_ticks = 0;
        routing_action_queue_push_front(path, &action);
    }

    // add start up acceleration data
    // next holds first node
    if (target_spd == start_spd) return dist[HASH(end_node)]; // no need to get to speed

    int32_t initial_start_time = get_time_from_acceleration(&spd_data, trn, SPD_STP, SPD_LO);
    int32_t secondary_start_time = get_time_from_acceleration(&spd_data, trn, SPD_LO, target_spd);

    if (start_spd == 0) {
        // to go from 0 -> 7 -> spd
        if (secondary_start_time == 0) { // no extra acceleration to target spd? (target_spd == SPD_LO)
            action.sensor_num = SENSOR_NONE;
            action.action_type = SPD_REACHED;
            action.action.spd = target_spd;
            action.info.delay_ticks = initial_start_time;
            routing_action_queue_push_front(speed_changes, &action);
        } else {
            action.sensor_num = SENSOR_NONE;
            action.action_type = SPD_REACHED;
            action.action.spd = target_spd;
            action.info.delay_ticks = secondary_start_time;
            routing_action_queue_push_front(speed_changes, &action);

            action.sensor_num = SENSOR_NONE;
            action.action_type = SPD_CHANGE;
            action.action.spd = target_spd;
            action.info.delay_ticks = initial_start_time;
            routing_action_queue_push_front(speed_changes, &action);
        }

        action.sensor_num = SENSOR_NONE;
        action.action_type = SPD_CHANGE;
        action.action.spd = SPD_LO;
        action.info.delay_ticks = 0;
        routing_action_queue_push_front(speed_changes, &action);
    } else {
        // only need to go from 7 -> spd
        action.sensor_num = SENSOR_NONE;
        action.action_type = SPD_REACHED;
        action.action.spd = target_spd;
        action.info.delay_ticks = secondary_start_time;
        routing_action_queue_push_front(speed_changes, &action);

        action.sensor_num = SENSOR_NONE;
        action.action_type = SPD_CHANGE;
        action.action.spd = target_spd;
        action.info.delay_ticks = 0;
        routing_action_queue_push_front(speed_changes, &action);
    }

    return dist[HASH(end_node)];
}

void plan_route(track_node *start_node, track_node *end_node,
    int16_t offset, int16_t trn, enum SPEEDS start_spd,
    enum SPEEDS target_spd,
    routing_action_queue *path, routing_action_queue *speed_changes) {
    // NOTE: by design, not possible to be unreachable and too short in
    // both reverse and forward directions

    // to prevent double copying and efficiency, favour straight route first
    if (plan_direct_route(start_node, end_node, offset,
        trn, start_spd, target_spd, true, path, speed_changes) >= 0)
        return;

    // try reverse path
    // omit first sensor
    if (plan_direct_route(start_node->reverse, end_node,
        offset - REVERSE_OFFSET,
        trn, start_spd, target_spd, false, path, speed_changes) < 0) {
        upanic("It really happened -- could not find suitable route");
    }

    // add reverse instructions at start and end
    routing_action action;
    action.sensor_num = SENSOR_NONE;
    action.action_type = SPD_CHANGE;
    action.action.spd = SP_REVERSE;
    action.info.delay_ticks = 0; // no delay for first reverse
    routing_action_queue_push_front(speed_changes, &action);

    // second reverse must happen after stop
    action.info.delay_ticks = RV_WAIT_TIME;
    routing_action_queue_push_back(speed_changes, &action);

    ULOG("taking reverse path\r\n");
}
