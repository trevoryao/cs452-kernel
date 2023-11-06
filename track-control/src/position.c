#include "position.h"

#include "speed.h"
#include "monitor.h"
#include "nameserver.h"
#include "uart-server.h"
#include "track.h"
#include "track-node.h"
#include "rpi.h"

extern speed_data spd_data;
extern track_node track[];

// timestamp is expected in clock_ticks
int trn_calculate_next_expected_time(trn_position *trn_pos, int8_t trainNo) {
    int train_hash = trn_hash(trainNo);
    // Case 1: no speed changes since last sensor 
    if (trn_pos->constant_speed_offset[train_hash] == 0) {
        int time = get_time_from_velocity_um(&spd_data, trainNo, trn_pos->distance_to_next_sensor[train_hash], trn_pos->current_train_speed[train_hash]);
        trn_pos->next_expected_clock_tick[train_hash] = time + trn_pos->last_seen_clock_tick[train_hash];

       
    } else {
        // Case 2.1 Constant speed BEFORE next sensor
        if (trn_pos->constant_speed_offset[train_hash] < trn_pos->distance_to_next_sensor[train_hash]) {
            // Calculate the difference
            int diff_distance = trn_pos->distance_to_next_sensor[train_hash] - trn_pos->constant_speed_offset[train_hash];

            int time_to_arrival = get_time_from_velocity_um(&spd_data, trainNo, diff_distance, trn_pos->current_train_speed[train_hash]);
            trn_pos->next_expected_clock_tick[train_hash] = time_to_arrival + trn_pos->constant_speed_clock_tick[train_hash];

            uart_printf(CONSOLE, "Time to next sensor: %d diff_distance, %d time_to_arrival, %d constant speed offset\r\n", diff_distance, time_to_arrival, trn_pos->constant_speed_clock_tick[train_hash]);
        } else {
            // MORE complicated -> tbd

        }

    }

    return trn_pos->next_expected_clock_tick[train_hash];
}

void trn_position_init(trn_position *trn_pos) {
    for (int i = 0; i < N_TRNS; i++){
        trn_pos->last_train_speed[i] = 0;
        trn_pos->current_train_speed[i] = 0;

        trn_pos->last_seen_clock_tick[i] = 0;
        trn_pos->next_expected_clock_tick[i] = 0;

        trn_pos->constant_speed_clock_tick[i] = 0;
        trn_pos->constant_speed_offset[i] = 0;

        trn_pos->distance_to_next_sensor[i] = 0;
    }

    trn_pos->monitor_tid = WhoIs(CONSOLE_SERVER_NAME);
}

//  Called on sensor trip -> updates the time
void trn_position_reached_next_sensor(trn_position *trn_pos, int8_t trainNo, uint32_t timestamp) {
    int train_hash = trn_hash(trainNo);
    if (trn_pos->distance_to_next_sensor[train_hash] != 0) {
        // calculate the difference
        int32_t diff = trn_pos->next_expected_clock_tick[train_hash] - timestamp;
        uart_printf(CONSOLE, "the calculated difference is %d\r\n", diff);
        // print_train_time(trn_pos->monitor_tid, trainNo, diff, sensor_mod, sensor_no);
    }

    // TODO: udapte the offsets + speeds
    
    if (trn_pos->constant_speed_offset[train_hash] < trn_pos->distance_to_next_sensor[train_hash]) {
        trn_pos->constant_speed_clock_tick[train_hash] = timestamp;
        trn_pos->constant_speed_offset[train_hash] = 0;
        trn_pos->last_train_speed[train_hash] = trn_pos->current_train_speed[train_hash];
    
    } else {
        trn_pos->constant_speed_offset[train_hash] -= trn_pos->distance_to_next_sensor[train_hash];

    }
    trn_pos->distance_to_next_sensor[train_hash] = 0;
    trn_pos->last_seen_clock_tick[train_hash] = timestamp;
}

void trn_position_update_train_speed(trn_position *trn_pos, int8_t trainNo, int8_t newSpeed, uint32_t timestamp) {
    int train_hash = trn_hash(trainNo);

    if (trn_pos->current_train_speed[train_hash] == 0) { 
        trn_pos->last_seen_clock_tick[train_hash] = timestamp;

        int constant_offset = get_distance_from_acceleration(&spd_data, trainNo, 0, newSpeed);
        uart_printf(CONSOLE, "Train starting with offset: %d \r\n", constant_offset);
        trn_pos->constant_speed_offset[train_hash] = constant_offset;
        trn_pos->constant_speed_clock_tick[train_hash] = get_time_from_acceleration(&spd_data, trainNo, 0, newSpeed) + timestamp;
        
        trn_pos->current_train_speed[train_hash] = newSpeed;
        trn_pos->last_train_speed[train_hash] = 0;
    
    } else if (trn_pos->current_train_speed[train_hash] != newSpeed) {

        // sanity check that it is acutally over the new constant speed
        if (timestamp < trn_pos->constant_speed_clock_tick[train_hash]) {
            uart_printf(CONSOLE, "Speed update to early - train has not yet reached old speed\r\n");
        }
        
        int oldSpeed = trn_pos->current_train_speed[train_hash];
        // update new constant position
        int constant_offset = get_distance_from_acceleration(&spd_data, trainNo, oldSpeed, newSpeed);
        int clock_offset = get_time_from_acceleration(&spd_data, trainNo, oldSpeed, newSpeed);

        int time_constant_travel = timestamp - trn_pos->constant_speed_clock_tick[train_hash];
        trn_pos->constant_speed_clock_tick[train_hash] = timestamp + clock_offset;
      
        // distance from last constant offset + time where train traveled with constant speed + new constant speedup
        trn_pos->constant_speed_offset[train_hash] = get_distance_from_velocity(&spd_data, trainNo, (time_constant_travel), oldSpeed) 
            + trn_pos->constant_speed_offset[train_hash] + constant_offset;


        // set speed constants
        trn_pos->last_train_speed[train_hash] = oldSpeed;
        trn_pos->current_train_speed[train_hash] = newSpeed;

    }

    uart_printf(CONSOLE, "Train speeding up at %d clocktick. Expected to reach new speed at %d mm and %d clocktick\r\n", timestamp, trn_pos->constant_speed_offset[train_hash], trn_pos->constant_speed_clock_tick[train_hash]);

    if (trn_pos->distance_to_next_sensor[train_hash] != 0) {
        trn_calculate_next_expected_time(trn_pos, trainNo);
    }


}

// expects new distance in um
void trn_position_update_next_expected_pos(trn_position *trn_pos, int8_t trainNo, uint32_t next_distance) {
    int train_hash = trn_hash(trainNo);

    if (trn_pos->distance_to_next_sensor[train_hash] != 0) {
        uart_printf(CONSOLE, "Train has not yet reached expected sensor");
        return;
    }

    trn_pos->distance_to_next_sensor[train_hash] = next_distance;
    int new_expected_time = trn_calculate_next_expected_time(trn_pos, trainNo);
    uart_printf(CONSOLE, "Calculated new Time %d\r\n", new_expected_time);
}





