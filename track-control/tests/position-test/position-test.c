#include "clock-server.h"
#include "console-server.h"
#include "marklin-server.h"
#include "task.h"
#include "time.h"
#include "uart-server.h"
#include "uassert.h"

#include "monitor.h"
#include "routing.h"
#include "speed-data.h"
#include "track.h"
#include "track-control-coordinator.h"
#include "track-data.h"
#include "train.h"

speed_data spd_data;
track_node track[TRACK_MAX];

void user_main(void) {
    // start up clock, uart servers
    Create(P_SERVER_HI, clockserver_main);

    uint16_t console_tid = Create(P_SERVER_LO, console_server_main);
    uint16_t marklin_tid = Create(P_SERVER_HI, marklin_server_main);

    speed_data_init(&spd_data);
    init_track_a(track);

    Create(P_SERVER_HI, track_control_coordinator_main);

    // initialisation commands
    init_monitor(console_tid);
    init_track(marklin_tid);
    WaitOutputEmpty(marklin_tid);

    // begin test
    // print out route first for reference
    routing_action_queue path;
    routing_action_queue_init(&path);
    routing_action_queue speed_changes;
    routing_action_queue_init(&speed_changes);

    track_node *start = &track[62];
    track_node *end = &track[53];

    uint8_t trn = 24;
    uint8_t spd = SPD_HI;

    Printf(console_tid, "Planning route %s -> %s\r\n", start->name, end->name);

    plan_route(start, end, 0, trn, SPD_STP, spd, &path, &speed_changes);

    routing_action action;

    Printf(console_tid, "***** Planned *****\r\n");

    Printf(console_tid, "Route:\r\n");
    while (!routing_action_queue_empty(&path)) {
        routing_action_queue_front(&path, &action);

        if (action.action_type == SWITCH) {
            uassert(action.info.delay_ticks == 0);
            Printf(console_tid, "Switch %d to %d (at Sensor %c%d)\r\n",
                action.action.sw.num, action.action.sw.dir,
                SENSOR_MOD(action.sensor_num) + 'A' - 1, SENSOR_NO(action.sensor_num));
        } else if (action.action_type == SENSOR) {
            Printf(console_tid, "Sensor %c%d -- dist to next: %d mm\r\n",
                SENSOR_MOD(action.sensor_num) + 'A' - 1, SENSOR_NO(action.sensor_num),
                action.info.dist);
        } else {
            upanic("Incorrect Action Type: %s\r\n", action);
        }

        routing_action_queue_pop_front(&path, NULL);
    }

    Printf(console_tid, "Speed Changes:\r\n");
    while (!routing_action_queue_empty(&speed_changes)) {
        routing_action_queue_front(&speed_changes, &action);
        uassert(action.action_type != SWITCH);

        char *action_str = (action.action_type == SPD_CHANGE) ? "Changed Speed to" : "Reached Speed";
        Printf(console_tid, "%s %d after %d ticks ",
            action_str, action.action.spd, action.info.delay_ticks);

        if (action.sensor_num != SENSOR_NONE) {
            Printf(console_tid, "(after Sensor %c%d)",
                SENSOR_MOD(action.sensor_num) + 'A' - 1, SENSOR_NO(action.sensor_num));
        }

        Puts(console_tid, "\r\n");
        routing_action_queue_pop_front(&speed_changes, NULL);
    }

    Printf(console_tid, "***** Start Planning *****\r\n");
    CreateControlledTrain(trn, spd, start, end, 0);
}