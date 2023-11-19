#ifndef __ROUTING_H__
#define __ROUTING_H__

#include <stdbool.h>
#include <stdint.h>

#include "deque.h"

#include "controller-consts.h"
#include "speed.h"

typedef struct track_node track_node;

/*
 * Interface for running Dijkstra's Algorithm
 * on the Track and returning route
 */

#define SENSOR_NONE -1

// a routing_action is a sensor at which an action must be undertaken
// i.e. throw an incoming switch, deaccelerate, reach constant speed, stop
// if sensor_num == SENSOR_NONE, then do action after delay_ticks.
// Otherwise, wait for sensor trigger and delay_ticks before doing action
// Note some actions (SPD_REACHED) may require no action

typedef struct routing_action {
    int16_t sensor_num;

    enum {
        SWITCH,
        SPD_CHANGE,
        SPD_REACHED,
        SENSOR
    } action_type;

    union {
        struct {
            uint8_t num;
            uint8_t dir;
        } sw;
        uint8_t spd;
        uint16_t total; // only for copying
    } action;

    union {
        uint32_t delay_ticks;
        int32_t dist; // mm
    } info;

} routing_action;

/* Queue for holding Routing Actions */

typedef struct routing_action_queue {
    deque q;
} routing_action_queue;

void routing_action_queue_init(routing_action_queue *raq);
void routing_action_queue_reset(routing_action_queue *raq);

bool routing_action_queue_empty(routing_action_queue *raq);
bool routing_action_queue_full(routing_action_queue *raq);
uint16_t routing_action_queue_size(routing_action_queue *raq);

void routing_action_queue_push_back(routing_action_queue *raq, routing_action *action);
void routing_action_queue_push_front(routing_action_queue *raq, routing_action *action);

void routing_action_queue_pop_front(routing_action_queue *raq, routing_action *action);
void routing_action_queue_pop_back(routing_action_queue *raq, routing_action *action);

void routing_action_queue_front(routing_action_queue *raq, routing_action *action);
void routing_action_queue_back(routing_action_queue *raq, routing_action *action);

typedef enum ROUTING_STATE {
    ERR_NO_ROUTE = -1,
    NORMAL_SEGMENT,
    FINAL_SEGMENT
} ROUTING_STATE;

typedef struct decision_pt {
    int16_t sensor_num;
    uint32_t ticks;
} decision_pt;

typedef struct route {
    ROUTING_STATE state;

    int32_t total_path_dist; // total distance on the path in mm (not just the segment)

    decision_pt decision_pt;

    routing_action_queue path;
    routing_action_queue speed_changes;
    deque segments;
} route;

void routing_actions_init(route *actions);

/*
 * Path Finding methods assume that train server has found
 * start & finish track_nodes
 *
 * All Path Finding Methods return the routing actions to the end
 * of the sector, and the decision point of the sector.
 *
 * A decision point is the latest time before the sector-end sensor
 * that we can not have a lock on the next sector.
 * If we reach this point without acquiring a lock on the track, the train
 * administrator is expected to perform an emergency stop.
 *
 * Otherwise, the routing algorithm assumes we have the lock and adds
 * instructions to throw any necessary switches in the segment to complete
 * the path.
 *
 * The Path Finding methods assume we already have a lock acquired
 * on the next segment, and include the next decision point.
 */

// assume starting spd = 0
// start_node is the first node on the path
// returns both a fwd
void plan_stopped_route(track_node *start_node, track_node *end_node,
    int16_t offset, int16_t trn, enum SPEEDS spd, int track_server_tid,
    route *fwd_route, route *rv_route);

// assumes already at spd
// will not reverse train mid-route
void plan_in_progress_route(track_node *start_node, track_node *end_node,
    int16_t offset, int16_t trn, enum SPEEDS spd, int track_server_tid,
    route *route);

#endif
