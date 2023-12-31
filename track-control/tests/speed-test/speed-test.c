#include "rpi.h"
#include "clock.h"
#include "speed.h"

void user_main(void) {
    uart_printf(CONSOLE, "this is test");

    struct speed_t speed_test;

    speed_t_init(&speed_test);

    int32_t dist_from_zero = get_distance_from_acceleration(&speed_test, 77, 0, 7);
    uart_printf(CONSOLE, "Train 77 takes %d um while acceleration from 0 to 7\r\n", dist_from_zero);

    int32_t dist_from_acc = get_distance_from_acceleration(&speed_test, 77, 7, 9);
    uart_printf(CONSOLE, "Train 77 takes %d um while acceleration from 7 to 9\r\n", dist_from_acc);
    dist_from_acc = get_distance_from_acceleration(&speed_test, 77, 9, 7);
    uart_printf(CONSOLE, "Train 77 takes %d um while acceleration from 9 to 7\r\n", dist_from_acc);

    int32_t time_from_acc = get_time_from_acceleration(&speed_test, 77, 7, 9);
    uart_printf(CONSOLE, "Train 77 takes %d clock ticks to acceleration from 7 to 9\r\n", time_from_acc);

    time_from_acc = get_time_from_acceleration(&speed_test, 77, 9, 7);
    uart_printf(CONSOLE, "Train 77 takes %d clock ticks to acceleration from 9 to 7\r\n", time_from_acc);


    // TODO set time in struct
    // TODO check distance
    speed_set(&speed_test, 77, 7);
    int32_t velocity_time = get_time_from_velocity(&speed_test, 77, 4700);
    uart_printf(CONSOLE, "Train 77 takes %d clock ticks to travel 4700 mm at speed 7\r\n", velocity_time);



}