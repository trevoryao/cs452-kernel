#include "rpi.h"
#include "clock.h"
#include "speed.h"

void user_main(void) {
    uart_printf(CONSOLE, "this is test");

    struct speed_t speed_test;

    speed_t_init(&speed_test);

    int32_t dist_from_acc = get_distance_from_acceleration(&speed_test, 77, 7, 9);
    uart_printf(CONSOLE, "Train 77 takes %d um while acceleration from 7 to 9", dist_from_acc);

    int32_t time_from_acc = get_time_from_acceleration(&speed_test, 77, 7, 9);
    uart_printf(CONSOLE, "Train 77 takes %d clock ticks to acceleration from 7 to 9", time_from_acc);


    // TODO set time in struct
    // TODO check distance
    int32_t velocity_time = get_time_from_velocity(&speed_test, 77, 100);



}