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

void routing_action_queue_back(routing_action_queue *raq, routing_action *action) {
    // no backwards iterators...
    routing_action_queue_pop_back(raq, action);
    routing_action_queue_push_back(raq, action);
}

void routing_actions_init(route *actions) {
    actions->state = NORMAL_SEGMENT;
    actions->total_path_dist = 0;
    actions->decision_pt.sensor_num = SENSOR_NONE;
    actions->decision_pt.ticks = 0;

    routing_action_queue_init(&actions->path);
    routing_action_queue_init(&actions->speed_changes);
    deque_init(&actions->segments, 3);
}

void routing_actions_reset(route *actions) {
    actions->state = NORMAL_SEGMENT;
    actions->total_path_dist = 0;
    actions->decision_pt.sensor_num = SENSOR_NONE;
    actions->decision_pt.ticks = 0;

    routing_action_queue_reset(&actions->path);
    routing_action_queue_reset(&actions->speed_changes);
    deque_reset(&actions->segments);
}

#define HASH(node) ((node) - track)

#define DIST_TRAVELLED(n1, n2) ((dist[HASH(n2)] - dist[HASH(n1)]) * MM_TO_UM)

#define SW_DELAY_TICKS 75
#define SPD_REACH_WINDOW 10
#define IN_USE_PENALTY 15 // length multiplier

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

static void gather_branches(route *route, deque *branches, track_node *node);

static const int32_t DECISION_PT_OFFSET = -200 * MM_TO_UM;
static const int32_t MIN_ROLL_DIST = 50 * MM_TO_UM; // um (5cm)

// main method for path finding
// assumes train server has found track node num in track[]
// for start/finish nodes
static void
plan_direct_route(track_node *start_node, track_node *end_node,
    int16_t offset, int16_t trn, enum SPEEDS start_spd,
    enum SPEEDS target_spd, bool include_first_node,
    int track_server_tid, route *route) {
    uassert(routing_action_queue_empty(&route->path));
    uassert(routing_action_queue_empty(&route->speed_changes));
    uassert(deque_empty(&route->segments));

    // ULOG("plan direct route from %s\r\n", start_node->name);

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
            end_node = node;
            // ULOG("Found path to %s\r\n", end_node->name);
            break; // reached!
        }

        if (dist[HASH(node)] == INT32_MAX) { // can't reach?
            // ULOG("node was unreachable\r\n");
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
                // must account for not visited case
                if (priority_queue_decrease(&pq, nbr, alt_path_weighted_dist)) {
                    weighted_dist[HASH(nbr)] = alt_path_weighted_dist;
                    dist[HASH(nbr)] = alt_path_dist;
                    prev[HASH(nbr)] = node;
                }
            }
        }
    }

    // backtrack to find route
    int32_t stopping_dist = get_stopping_distance(&spd_data, trn, target_spd);
    int32_t accel_dist = get_distance_from_acceleration(&spd_data, trn,
        start_spd, target_spd);

    // ULOG("stopping_dist: %dmm accel_dist: %dmm\r\n", stopping_dist / MM_TO_UM, accel_dist / MM_TO_UM);

    // queueing branch nodes for each segment
    // futher along nodes are at the front, closer nodes are at the back
    deque branches;
    deque_init(&branches, 7);

    // constants for determining what is next in the branch deque
    #define BR_NODE -1
    #define SW_ANCHOR -2

    routing_action action; // used for pushing back actions

    track_node *next_sensor = NULL;
    track_node *next = NULL;
    track_node *node = end_node;

    // adjustable end point for stopping early (in case of reversing early)
    track_node *end = include_first_node ? NULL : start_node;

    // use as a stack to get full path from start to end, don't want to push full path onto route->path
    routing_action_queue sensor_path;
    routing_action_queue_init(&sensor_path);

    // iterate all the way back to wherever our root is
    while (node != end) {
        ULOG("backtracking %s\r\n", node->name);

        if (node->type == NODE_BRANCH) {
            int8_t branch_dir;
            if (next) { // only if we are stopping on a branch node
                branch_dir = (node->edge[DIR_CURVED].dest == next) ? DIR_CURVED : DIR_STRAIGHT;
            } else {
                branch_dir = DIR_STRAIGHT; // default straight
            }

            deque_push_back(&branches, branch_dir); // branch_dir
            deque_push_back(&branches, HASH(node)); // node
            deque_push_back(&branches, BR_NODE); // type
        } else if (node->type == NODE_SENSOR) {
            // add all branches to their proceeding sensor
            deque_push_back(&branches, HASH(node)); // node
            deque_push_back(&branches, SW_ANCHOR); // type

            // add distance info
            // deliberately add last node without any distance to avoid edge cases below
            action.sensor_num = node->num;
            action.action_type = SENSOR;
            action.action.total = 0;
            action.info.dist = (next_sensor != NULL) ?
                (DIST_TRAVELLED(node, next_sensor) / MM_TO_UM) : 0;
            routing_action_queue_push_front(&sensor_path, &action);

            next_sensor = node;
        }

        next = node;
        node = prev[HASH(next)];
    }

    // get starting node of our path
    routing_action_queue_pop_front(&sensor_path, &action);
    node = &track[action.sensor_num];

    // save for calculations
    track_node *true_starting_node = node;

    // ULOG("true starting node: %s\r\n", true_starting_node->name);

    routing_action_queue_push_back(&route->path, &action); // return to user in other order
    ULOG("pushing back true starting node %s\r\n", track[action.sensor_num].name);

    // is the stopping node our starting node?
    // must get next node

    // since we add destination node, can assume always non-empty
    // similar for all other cases below
    routing_action_queue_front(&sensor_path, &action); // node->next_sensor
    next_sensor = &track[action.sensor_num];

    if (start_spd == SPD_STP && (DIST_TRAVELLED(true_starting_node, end_node) < accel_dist + MIN_ROLL_DIST + stopping_dist || next_sensor == end_node)) {
        uart_printf(CONSOLE, "Short move! -- Not yet implemented\r\n");

        route->state = ERR_NO_ROUTE;
        return;
    }

    // if next sensor is less than the stopping distance, then we are the
    // first one more than stopping distance = > stopping_node
    // ULOG("[routing] stopping checking %dmm (%s -> %s)\r\n", (DIST_TRAVELLED(next_sensor, end_node) + (offset * MM_TO_UM)) / MM_TO_UM, next_sensor->name, end_node->name);

    // ULOG("[routing] next_sensor dist: %dmm end_node dist: %dmm \r\n", dist[HASH(next_sensor)], dist[HASH(end_node)]);

    if (DIST_TRAVELLED(next_sensor, end_node) + (offset * MM_TO_UM) < stopping_dist) {
        // stopping node
        // combine all our nodes into one single queue on route->path
        // segment change is now forward direction

        // ULOG("[routing] Stopping case\r\n");

        // already popped off for first sensor, so weird iteration
        for (;;) {
            if (node == end_node) {
                // add speed info
                int32_t sensor_dist = DIST_TRAVELLED(true_starting_node, end_node) + (offset * MM_TO_UM);

                action.sensor_num = true_starting_node->num;
                action.action_type = SPD_CHANGE;
                action.action.spd = SPD_STP;
                action.info.delay_ticks = calculate_stopping_delay(trn, stopping_dist, sensor_dist, target_spd);
                routing_action_queue_push_front(&route->speed_changes, &action);

                // gather all the branches
                gather_branches(route, &branches, node);

                route->state = FINAL_SEGMENT;

                // pop off final end node
                routing_action_queue_pop_back(&route->path, NULL);
                break;
            }

            // don't need to add for last one
            routing_action_queue_pop_front(&sensor_path, &action);
            routing_action_queue_push_back(&route->path, &action); // push back for next itr
            next_sensor = &track[action.sensor_num];

            // check for segment change
            if (node->segmentId != next_sensor->segmentId) {
                deque_push_back(&route->segments, node->segmentId);
            }

            node = next_sensor; // prime for next itr
        }
    } else if (start_spd == SPD_STP) {
        // check short move:
        // a short move can only happen if we cannot have time to accelerate and get up to speed and then stop on our path
        // in either case, if within short move params, we do a short move
        // if (DIST_TRAVELLED(true_starting_node, end_node) < accel_dist + MIN_ROLL_DIST + stopping_dist) {
        //     ULOG("Short move! -- Not yet implemented\r\n");

        //     for (;;) {
        //         if (node == end_node) {
        //             // add speed info
        //             action.sensor_num = SENSOR_NONE;
        //             action.action_type = SPD_CHANGE;
        //             action.action.spd = SPD_STP;
        //             action.info.delay_ticks = get_short_move_delay(&spd_data, trn,
        //                 DIST_TRAVELLED(true_starting_node, end_node));
        //             routing_action_queue_push_front(&route->speed_changes, &action);

        //             action.sensor_num = SENSOR_NONE;
        //             action.action_type = SPD_CHANGE;
        //             action.action.spd = SPD_LO; // short move speed
        //             action.info.delay_ticks = 0;
        //             routing_action_queue_push_front(&route->speed_changes, &action);

        //             // gather all the branches
        //             gather_branches(route, &branches, node);

        //             route->state = FINAL_SEGMENT;

        //             // no decision point
        //             route->decision_pt.sensor_num = SENSOR_NONE;

        //             route->state = ERR_NO_ROUTE; // don't support (for now)
        //             route->total_path_dist = dist[HASH(end_node)];
        //             return;
        //         }

        //         // don't add any sensor nodes to wait on
        //         // positioning doesn't apply
        //         routing_action_queue_pop_front(&sensor_path, &action);
        //         next_sensor = &track[action.sensor_num];

        //         // check for segment change
        //         if (node->segmentId != next_sensor->segmentId) {
        //             deque_push_back(&route->segments, node->segmentId);
        //         }

        //         node = next_sensor; // prime for next itr
        //     }
        // }

        // travel to the end of our acceleration distance
        for (;;) {
            // ULOG("[routing] accel checking %dmm (%s)\r\n", (DIST_TRAVELLED(true_starting_node, node) + (offset * MM_TO_UM)) / MM_TO_UM, node->name);

            // node must be the first node past the accel_dist
            if (DIST_TRAVELLED(true_starting_node, node) + (offset * MM_TO_UM) > accel_dist) {
                ULOG("node %s is our accel_node\r\n", node->name);
                // add speed info
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

                // gather all the branches until we hit our current node
                gather_branches(route, &branches, node);

                route->state = NORMAL_SEGMENT;
                break;
            }

            routing_action_queue_pop_front(&sensor_path, &action);
            routing_action_queue_push_back(&route->path, &action); // push back for next itr
            next_sensor = &track[action.sensor_num];
            ULOG("pushed back %s\r\n", next_sensor->name);

            // check for segment change
            if (node->segmentId != next_sensor->segmentId) {
                deque_push_back(&route->segments, node->segmentId);
            }

            node = next_sensor; // prime for next itr
        }
    } else {
        // constant speed case, iterate until we have a segment change
        // ULOG("CONSTANT CASE\r\n");
        for (;;) {
            routing_action_queue_pop_front(&sensor_path, &action);
            routing_action_queue_push_back(&route->path, &action); // push back for next itr
            // ULOG("pushing back %s (%d total)\r\n", track[action.sensor_num].name, routing_action_queue_size(&route->path));

            next_sensor = &track[action.sensor_num];

            // check for segment change
            if (node->segmentId != next_sensor->segmentId) {
                deque_push_back(&route->segments, node->segmentId);
                node = next_sensor; // for tracking where we end

                // gather all the branches until we hit our current node
                gather_branches(route, &branches, node);

                route->state = NORMAL_SEGMENT;
                break;
            }

            node = next_sensor; // prime for next itr
        }
    }

    // in all cases, node is now the last node that we will travel in this itr
    // calculate next decision point
    if (node == end_node) {
        // stopping
        // ULOG("[routing] no decision point for stopping\r\n");
        route->decision_pt.sensor_num = SENSOR_NONE;
    } else {
        int32_t decision_dist = DIST_TRAVELLED(true_starting_node,
            node) + DECISION_PT_OFFSET;

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
        route->decision_pt.sensor_num = true_starting_node->num;
        route->decision_pt.ticks = decision_delay_time;
    }

    route->total_path_dist = dist[HASH(end_node)];
    // ULOG("Returning state=%d\r\n", route->state);
}

static void gather_branches(route *route, deque *branches, track_node *node) {
    routing_action action;

    while (!deque_empty(branches)) {
        // pushes further along nodes first, closer nodes last (front of path)
        if (deque_pop_back(branches) == SW_ANCHOR) {
            ULOG("checking %s\r\n", track[deque_back(branches)].name);
            if ((int64_t)deque_pop_back(branches) == HASH(node)) {
                break; // reached ourselves
            }

        } else { // branch node
            track_node *branch_node = &track[deque_pop_back(branches)];
            int8_t branch_dir = deque_pop_back(branches);

            action.sensor_num = SENSOR_NONE; // throw immediately
            action.action_type = SWITCH;
            action.action.sw.num = branch_node->num;
            action.action.sw.dir = (branch_dir == DIR_CURVED) ? CRV : STRT;
            action.info.delay_ticks = 0;

            routing_action_queue_push_front(&route->path, &action);
        }
    }
}

void plan_in_progress_route(track_node *start_node, track_node *end_node,
    int16_t offset, int16_t trn, enum SPEEDS spd, int track_server_tid,
    route *route) {
    plan_direct_route(start_node, end_node, offset, trn, spd, spd, true, track_server_tid, route);
}

void plan_stopped_route(track_node *start_node, track_node *end_node,
    int16_t offset, int16_t trn, enum SPEEDS spd, int track_server_tid,
    route *fwd_route, route *rv_route) {
    ULOG("Forward Route:\r\n");
    plan_direct_route(start_node, end_node, offset, trn, SPD_STP, spd, true, track_server_tid, fwd_route);

    ULOG("Reverse Route:\r\n");
    plan_direct_route(start_node->reverse, end_node, offset, trn, SPD_STP, spd, false, track_server_tid, rv_route);
    // reverse in the route?
}
