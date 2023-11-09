#include "position.h"

#include "speed.h"
#include "monitor.h"
#include "nameserver.h"
#include "uart-server.h"
#include "track.h"
#include "track-node.h"
#include "rpi.h"
#include "speed-data.h"
#include "clock.h"

extern speed_data spd_data;
extern track_node track[];

void trn_position_init(struct trn_position *trn_pos) {
    for (int i = 0; i < N_TRNS; i++){
        trn_pos->current_train_speed[i] = 0;
        trn_pos->distance_to_next_sensor[i] = 0;
        trn_pos->distance_to_next_next_sensor[i] = 0;
        trn_pos->last_timestamp[i] = 0;
    }

    trn_pos->monitor_tid = WhoIs(CONSOLE_SERVER_NAME);
    trn_pos->clock_tid = WhoIs(CLOCK_SERVER_NAME);
}

//  Called on sensor trip -> updates the time
void trn_position_reached_next_sensor(struct trn_position *trn_pos, int8_t trainNo) {
    int train_hash = trn_hash(trainNo);
    
    // get timestamp
    int32_t curr_time = Time(trn_pos->clock_tid);

    if (trn_pos->last_timestamp[train_hash] == 0) {
        trn_pos->last_timestamp[train_hash] = curr_time;

    } else {
        // get expected time
        int64_t expected_time = get_time_from_velocity_um(&spd_data, trainNo, trn_pos->distance_to_next_sensor[train_hash], trn_pos->current_train_speed[train_hash]);

        int64_t actual_time = curr_time - trn_pos->last_timestamp[train_hash];
        int64_t difference = expected_time - actual_time;
        update_sensor_prediction(trn_pos->monitor_tid, trainNo, trn_pos->current_train_speed[train_hash], difference);

        // set new timestamp
        trn_pos->last_timestamp[train_hash] = curr_time;
    }

}

void trn_position_update_train_speed(trn_position *trn_pos, int8_t trainNo, int8_t newSpeed) {
    int train_hash = trn_hash(trainNo);
    trn_pos->current_train_speed[train_hash] = newSpeed;
}

// expects new distance in um
void trn_position_update_next_expected_pos(trn_position *trn_pos, int8_t trainNo, uint32_t next_distance) {
    int train_hash = trn_hash(trainNo);
    if (next_distance == 0) {
        uart_printf(CONSOLE, "Ignoring zero distance");
        return;
    }

    trn_pos->distance_to_next_sensor[train_hash] = trn_pos->distance_to_next_next_sensor[train_hash];
    trn_pos->distance_to_next_next_sensor[train_hash] = next_distance;
}
