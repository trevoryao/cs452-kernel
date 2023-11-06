#include "rpi.h"
#include "uassert.h"

#include "clock-server.h"
#include "controller-consts.h"
#include "routing.h"
#include "speed-data.h"
#include "track-data.h"
#include "train.h"

#include "sys-clock.h"

speed_data spd_data;
track_node track[TRACK_MAX];

void user_main(void) {
    speed_data_init(&spd_data);
    init_track_b(track);

    routing_action_queue path;
    routing_action_queue_init(&path);
    routing_action_queue speed_changes;
    routing_action_queue_init(&speed_changes);

    uart_printf(CONSOLE, "Planning route...\r\n");

    // A1 -> C3
    uint64_t ticks = get_curr_ticks();
    plan_route(&track[0], &track[34], 0, 77, SPD_STP, SPD_LO, &path, &speed_changes);
    ticks = get_curr_ticks() - ticks;

    uart_printf(CONSOLE, "Run Time (Ticks) %u\r\n", ticks);

    routing_action action;

    uart_printf(CONSOLE, "***** Planned *****\r\n");

    uart_printf(CONSOLE, "Route:\r\n");
    while (!routing_action_queue_empty(&path)) {
        routing_action_queue_front(&path, &action);

        if (action.action_type == SWITCH) {
            uassert(action.info.delay_ticks == 0);
            uart_printf(CONSOLE, "Switch %d to %d (at Sensor %c%d)\r\n",
                action.action.sw.num, action.action.sw.dir,
                SENSOR_MOD(action.sensor_num) + 'A' - 1, SENSOR_NO(action.sensor_num));
        } else if (action.action_type == SENSOR) {
            uart_printf(CONSOLE, "Sensor %c%d -- dist to next: %d mm\r\n",
                SENSOR_MOD(action.sensor_num) + 'A' - 1, SENSOR_NO(action.sensor_num),
                action.info.dist);
        } else {
            upanic("Incorrect Action Type: %s\r\n", action);
        }

        routing_action_queue_pop_front(&path, NULL);
    }

    uart_printf(CONSOLE, "Speed Changes:\r\n");
    while (!routing_action_queue_empty(&speed_changes)) {
        routing_action_queue_front(&speed_changes, &action);
        uassert(action.action_type != SWITCH);

        char *action_str = (action.action_type == SPD_CHANGE) ? "Changed Speed to" : "Reached Speed";
        uart_printf(CONSOLE, "%s %d after %d ticks ",
            action_str, action.action.spd, action.info.delay_ticks);

        if (action.sensor_num != SENSOR_NONE) {
            uart_printf(CONSOLE, "(after Sensor %c%d)",
                SENSOR_MOD(action.sensor_num) + 'A' - 1, SENSOR_NO(action.sensor_num));
        }

        uart_puts(CONSOLE, "\r\n");
        routing_action_queue_pop_front(&speed_changes, NULL);
    }

    Create(P_SERVER_HI, clockserver_main);

    uart_printf(CONSOLE, "***** Actual *****\r\n");
    CreateControlledTrain(77, SPD_LO, &track[0], &track[34], 0);
}
