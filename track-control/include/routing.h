#ifndef __ROUTING_H__
#define __ROUTING_H__

#include <stdbool.h>
#include <stdint.h>

#include "deque.h"

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

// main method for path finding
// assumes train server has found track node num in track[]
// for start/finish nodes
void plan_route(track_node *start_node, track_node *end_node,
    int16_t offset, int16_t trn, enum SPEEDS start_spd,
    enum SPEEDS target_spd,
    routing_action_queue *path, routing_action_queue *speed_changes);

#endif
