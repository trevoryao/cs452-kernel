#include "rpi.h"

#include "uassert.h"
#include "routing.h"
#include "speed-data.h"
#include "track-data.h"

speed_data spd_data;
track_node track[TRACK_MAX];

#define SENSOR_MOD(num) ((num) / 16) + 'A'
#define SENSOR_NO(num) ((num) % 16) + 1

void user_main(void) {
    speed_data_init(&spd_data);
    init_track_b(track);

    routing_action_queue path;
    routing_action_queue_init(&path);
    routing_action_queue speed_changes;
    routing_action_queue_init(&speed_changes);

    plan_route(NULL, NULL, 0, 77, SPD_STP, SPD_HI, &path, &speed_changes);

    routing_action action;

    uart_printf(CONSOLE, "Route:\r\n");
    while (!routing_action_queue_empty(&path)) {
        routing_action_queue_pop_front(&path, &action);
        uassert(action.action_type == SWITCH);
        uassert(action.delay_ticks == 0);

        uart_printf(CONSOLE, "Switch %d (at Sensor %c%d)\r\n",
            action.action.sw_num, SENSOR_MOD(action.sensor_num), SENSOR_NO(action.sensor_num));
    }

    uart_printf(CONSOLE, "Speed Changes:\r\n");
    while (!routing_action_queue_empty(&speed_changes)) {
        routing_action_queue_pop_front(&speed_changes, &action);
        uassert(action.action_type != SWITCH);

        char *action_str = (action.action_type == SPD_CHANGE) ? "Changed Speed to" : "Reached Speed";
        uart_printf(CONSOLE, "%s %d after %d ticks ",
            action_str, action.action.spd, action.delay_ticks);

        if (action.sensor_num != SENSOR_NONE) {
            uart_printf(CONSOLE, "(after Sensor %c%d)",
                SENSOR_MOD(action.sensor_num), SENSOR_NO(action.sensor_num));
        }

        uart_puts(CONSOLE, "\r\n");
    }
}