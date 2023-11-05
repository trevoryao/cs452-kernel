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
int trn_calculate_next_expected_time(trn_position *trn_pos, int8_t trainNo, int8_t newSpeed, uint32_t timestamp) {
    int train_hash = trn_hash(trainNo);

    // distance in mm
    int dist = trn_pos->distance_to_next_sensor[train_hash];

    if (dist == 0) {
        return 0;
    }

    int expected_arrival = 0;

   
    if (trn_pos->last_train_speed[train_hash] == newSpeed) {
        // Case 0: constant speed -> no speed change
        
        // returns time in clock_ticks
        int time_to_arrival = get_time_from_velocity(&spd_data, trainNo, dist, newSpeed);

        if (trn_pos->constant_speed_clock_tick[train_hash] == 0) {
            expected_arrival = trn_pos->last_seen_clock_tick[train_hash] + time_to_arrival;
        } else {
            expected_arrival = trn_pos->constant_speed_clock_tick[train_hash] + time_to_arrival;
        }

    } else {
        int dist_travelled_since_last_sensor = 0;

        int constant_time = (trn_pos->constant_speed_clock_tick[train_hash] == 0) ? 
            trn_pos->last_seen_clock_tick[train_hash] : trn_pos->constant_speed_clock_tick[train_hash];

        // first assuming constant speed from last clock tick to now
        constant_time = timestamp - constant_time;
        if (constant_time != 0) {
            dist_travelled_since_last_sensor = get_distance_from_velocity(&spd_data, trainNo, constant_time, trn_pos->last_train_speed[train_hash]);
        } 

        // Calculate the next time step when achieving constant speed 
        int next_constant_time = get_time_from_acceleration(&spd_data, trainNo, trn_pos->last_train_speed[train_hash], newSpeed);
        next_constant_time += timestamp;
        
        // Calculate distance traveled till then
        int offset_to_next_constant_speed = get_distance_from_acceleration(&spd_data, trainNo, trn_pos->last_train_speed[train_hash], newSpeed);
        offset_to_next_constant_speed += dist_travelled_since_last_sensor;


        

        // Use the data to calculate next sensor activation

        // Case 1: Sensor is after reaching constant speed
        if (offset_to_next_constant_speed < dist) {
            int constant_distance_end = dist - offset_to_next_constant_speed;
            int restTime = get_time_from_velocity(&spd_data, trainNo, constant_distance_end, newSpeed);
            expected_arrival = next_constant_time + restTime;
        } else {
            // Case 2: Sensor hit is before reaching constant speed
            // TODO: not yet implemented
            expected_arrival = 0;
        }

        // set data in structure
        trn_pos->constant_speed_clock_tick[train_hash] = next_constant_time;
        trn_pos->constant_speed_offset[train_hash] = offset_to_next_constant_speed;
        trn_pos->last_train_speed[train_hash] = newSpeed;

    }
    // add possible time from previous acceleration
    trn_pos->next_expected_clock_tick[train_hash] = expected_arrival;
    return expected_arrival;
}

void trn_position_init(trn_position *trn_pos) {
    for (int i = 0; i < N_TRNS; i++){
        trn_pos->last_train_speed[i] = 0;
        
        trn_pos->last_seen_clock_tick[i] = 0;
        trn_pos->next_expected_clock_tick[i] = 0;

        trn_pos->constant_speed_clock_tick[i] = 0;
        trn_pos->constant_speed_offset[i] = 0;

        trn_pos->distance_to_next_sensor[i] = 0;
    }

    trn_pos->monitor_tid = WhoIs(CONSOLE_SERVER_NAME);
}

//  Called on sensor trip -> updates the time 
void trn_position_update_train_pos(trn_position *trn_pos, int8_t trainNo, int8_t sensor_mod, int8_t sensor_no, uint32_t timestamp) {
    int train_hash = trn_hash(trainNo);
    if (trn_pos->distance_to_next_sensor[train_hash] != 0) {
        // calculate the difference 
        int32_t diff = trn_pos->next_expected_clock_tick[train_hash] - timestamp;
        print_train_time(trn_pos->monitor_tid, trainNo, diff, sensor_mod, sensor_no);
    }

    // update the position of the train

    trn_pos->last_seen_clock_tick[train_hash] = timestamp;
}

void trn_position_update_train_speed(trn_position *trn_pos, int8_t trainNo, int8_t newSpeed, uint32_t timestamp) {
    int train_hash = trn_hash(trainNo);

    trn_calculate_next_expected_time(trn_pos, trainNo, newSpeed, timestamp);

    trn_pos->last_train_speed[train_hash] = newSpeed;

}

void trn_position_update_next_expected_pos(trn_position *trn_pos, int8_t trainNo, uint32_t next_distance, uint32_t timestamp) {
    int train_hash = trn_hash(trainNo);

    trn_pos->distance_to_next_sensor[train_hash] = next_distance;
    int new_expected_time = trn_calculate_next_expected_time(trn_pos, trainNo, trn_pos->last_train_speed[train_hash], timestamp);
    uart_print(CONSOLE, "Calculated new Time %d", new_expected_time);
}





