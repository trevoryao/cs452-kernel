#ifndef __POSITION_H__
#define __POSITION_H__

#include <stdbool.h>
#include <stdint.h>

#include "speed-data.h"

typedef struct sensor_queue sensor_queue;

/*
 * This interface provides predictions on when the specified train
 * will reach the next sensor on the track.
 * Updates the UI
 */

enum POS_STATE {
    POS_STATE_WAITING,
    POS_STATE_HIT
};

typedef struct train_data_pos {
    // speed metadata
    uint8_t spd_init;
    uint8_t spd_final; // equal to spd_init if constant

    // state (FSM)
    enum POS_STATE state;

    // queued speed reset/change (if comes before next wait)
    // only at most 2 (discard earlier speed changes)
    #define NO_QUEUE -1
    int8_t queued_speed_reset;
    int8_t queued_speed_change;
    uint32_t queued_speed_change_time;

    // TCC inserts waiting sensors into queue for us
    // however we may need to adjust our prediction before hitting
    // the sensor
    uint16_t last_waited_sensor_mod;
    uint16_t last_waited_sensor_no;
    uint16_t last_waited_trn_tid;

    /***** used in constant/non-constant *****/

    // distance from last sensor to prediction sensor
    // or DIST_NONE
    int32_t sensor_dist;

    // time at which the train crossed prev sensor (or start pt)
    // TIME_NONE if not set (i.e. train is stopped)
    int64_t last_timestamp;

    // save the last estimated time calculated
    // returned by subsequent set_sensor call (for TCC stuffs)
    // TIME_NONE if not set (train stopped)
    int64_t estimated_time;

    // estimated time for upcoming sensor
    // set after prediction is made
    // used for comparing difference only
    int64_t old_estimated_time;

    /***** used only in non-constant *****/

    // flags
    bool startup;

    // total estimated distance over speed change
    int32_t total_dist;

    // distance/time travelled so far
    int32_t travelled_dist;
    uint32_t travelled_time;
} train_data_pos;

typedef struct trn_position {
    // current speed and the speed before
    struct train_data_pos data[N_TRNS];

    uint16_t monitor_tid;
    uint16_t clock_tid;
    sensor_queue *sq; // for inserting/adjusting
} trn_position;

void trn_position_init(trn_position *pos, sensor_queue *sq);
void trn_position_reset(trn_position *pos, uint8_t trn);

// Called by TCC when speed change is requested
void trn_position_update_speed(trn_position *pos, uint8_t trn, uint8_t spd,
    uint32_t update_spd_time);

// Called by TCC when sensor is waited on by a train
// Performs actual prediction calculation
// returns expected timeout or TIME_NONE if unknown
int64_t trn_position_set_sensor(trn_position *pos, uint8_t trn, int32_t dist,
    uint16_t sensor_mod, uint16_t sensor_no, uint16_t trn_tid);

// Called by TCC when waited on sensor is activated
// Prints difference between prediction and reality
void trn_position_reached_sensor(trn_position *pos, uint8_t trn,
    uint32_t activation_time);

#endif
