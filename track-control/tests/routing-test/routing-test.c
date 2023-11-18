#include "rpi.h"
#include "uassert.h"

#include "clock-server.h"
#include "controller-consts.h"
#include "routing.h"
#include "speed-data.h"
#include "track-data.h"
#include "track-server.h"
#include "train.h"

#include "sys-clock.h"

speed_data spd_data;
track_node track[TRACK_MAX];

static track_node *print_segment_route(routing_actions *route) {
    if (route->state == ERR_NO_ROUTE) {
        uart_printf(CONSOLE, "ERROR: no route found\r\n");
        return NULL;
    }

    routing_action action;

    uart_printf(CONSOLE, "Route:\r\n");
    while (!routing_action_queue_empty(&route->path)) {
        routing_action_queue_front(&route->path, &action);

        if (action.action_type == SWITCH) {
            uassert(action.info.delay_ticks == 0);

            uart_printf(CONSOLE, "Switch %d to %d",
                action.action.sw.num, action.action.sw.dir);

            if (action.sensor_num != SENSOR_NONE) {
                uart_printf(CONSOLE, " (at Sensor %c%d)",
                    SENSOR_MOD(action.sensor_num) + 'A' - 1, SENSOR_NO(action.sensor_num));
            }

            uart_printf(CONSOLE, "\r\n");
        } else if (action.action_type == SENSOR) {
            uart_printf(CONSOLE, "Sensor %c%d -- dist to next: %d mm\r\n",
                SENSOR_MOD(action.sensor_num) + 'A' - 1, SENSOR_NO(action.sensor_num),
                action.info.dist);
        } else {
            upanic("Incorrect Action Type: %s\r\n", action);
        }

        routing_action_queue_pop_front(&route->path, NULL);
    }

    track_node *segment_end = &track[action.sensor_num];

    uart_printf(CONSOLE, "Speed Changes:\r\n");
    while (!routing_action_queue_empty(&route->speed_changes)) {
        routing_action_queue_front(&route->speed_changes, &action);

        char *action_str;

        switch (action.action_type) {
            case SPD_CHANGE:
                uart_printf(CONSOLE, "Changed Speed to %d after %d ticks",
                    action.action.spd, action.info.delay_ticks);
                break;
            case SPD_REACHED:
                uart_printf(CONSOLE, "Reached Speed %d after %d ticks",
                    action.action.spd, action.info.delay_ticks);
                break;
            case DECISION_PT:
                uassert(action.sensor_num != SENSOR_NONE)
                uart_printf(CONSOLE, "Decision point: %d ticks",
                    action.info.delay_ticks);
                break;
            default:
                upanic("Incorrect Action Type: %s\r\n", action);
                break;
        }

        if (action.sensor_num != SENSOR_NONE) {
            uart_printf(CONSOLE, " (after Sensor %c%d)",
                SENSOR_MOD(action.sensor_num) + 'A' - 1, SENSOR_NO(action.sensor_num));
        }

        uart_puts(CONSOLE, "\r\n");
        routing_action_queue_pop_front(&route->speed_changes, NULL);
    }

    uart_printf(CONSOLE, "Segments:");
    while (!deque_empty(&route->segments)) {
        uart_printf(CONSOLE, " %d", deque_pop_front(&route->segments));
    }
    uart_printf(CONSOLE, "\r\n");

    return segment_end;
}

void user_main(void) {
    speed_data_init(&spd_data);
    init_track_b(track);

    int track_server_tid = Create(P_SERVER_HI, track_server_main);

    routing_actions fwd_route;
    routing_actions_init(&fwd_route);

    routing_actions rv_route;
    routing_actions_init(&rv_route);

    routing_actions *chosen_route;

    track_node *start = &track[45];
    track_node *end = &track[1];
    uint8_t trn = 77;
    uint8_t spd = SPD_MED;

    uart_printf(CONSOLE, "Planning route %s -> %s\r\n", start->name, end->name);

    track_node *next_segment = start;

    // edge case => from stationary
    uint64_t ticks = get_curr_ticks();
    plan_stopped_route(start, end, 0, trn, spd, track_server_tid, &fwd_route, &rv_route);
    ticks = get_curr_ticks() - ticks;

    uart_printf(CONSOLE, "From Stationary (Computation time %d ticks)\r\n", ticks);

    uart_printf(CONSOLE, "Forward:\r\n");
    track_node *fwd_next_segment = print_segment_route(&fwd_route);

    uart_printf(CONSOLE, "Backward:\r\n");
    track_node *rv_next_segment = print_segment_route(&rv_route);

    // determine which route to take
    if (fwd_next_segment != NULL) {
        next_segment = fwd_next_segment;
        chosen_route = &fwd_route;
    } else if (rv_next_segment != NULL) {
        next_segment = rv_next_segment;
        chosen_route = &rv_route;
    } else {
        upanic("ERROR: no route found!");
    }

    // now to arbitrary case
    while (next_segment != end) {
        ticks = get_curr_ticks();
        plan_in_progress_route(start, end, 0, trn, spd, track_server_tid, chosen_route);
        ticks = get_curr_ticks() - ticks;
        uart_printf(CONSOLE, "\r\nNext Segment (Computation time %d ticks)\r\n", ticks);
        next_segment = print_segment_route(chosen_route);
    }
}
