#include "position.h"

#include "clock.h"
#include "nameserver.h"
#include "uart-server.h"
#include "uassert.h"
#include "util.h"

#include "monitor.h"
#include "track-data.h"
#include "sensor-queue.h"

extern speed_data spd_data;
extern track_node track[];

static void trn_position_reset_hash(trn_position *pos, uint8_t i) {
    memset(&pos->data[i], 0, sizeof(train_data_pos));
    pos->data[i].queued_speed_reset = NO_QUEUE;
    pos->data[i].queued_speed_change = NO_QUEUE;
    pos->data[i].queued_speed_change_time = TIME_NONE;
    pos->data[i].startup = true;
    pos->data[i].last_timestamp = TIME_NONE;
    pos->data[i].estimated_time = TIME_NONE;
    pos->data[i].old_estimated_time = TIME_NONE;
}

void trn_position_init(struct trn_position *pos, sensor_queue *sq) {
    for (int i = 0; i < N_TRNS; i++)
        trn_position_reset_hash(pos, i);

    pos->monitor_tid = WhoIs(CONSOLE_SERVER_NAME);
    pos->clock_tid = WhoIs(CLOCK_SERVER_NAME);
    pos->sq = sq;
}

void trn_position_reset(trn_position *pos, uint8_t trn) {
    int8_t trn_idx = trn_hash(trn);
    uassert(trn_idx >= 0);
    trn_position_reset_hash(pos, trn_idx);
}

static void
trn_position_end_accel(train_data_pos *trn_data, int const_spd) {
    ULOG("[pos] const %d\r\n", const_spd);
    trn_data->spd_init = const_spd;
    trn_data->spd_final = const_spd;
    // keep state, sensor_dist, last_timestamp, estimated time
    trn_data->queued_speed_reset = NO_QUEUE;
    trn_data->startup = true;
    trn_data->total_dist = 0;
    trn_data->travelled_dist = 0;
    trn_data->travelled_time = 0;
}

static void
trn_position_recalculate_starting_estimate(trn_position *pos, uint8_t trn,
    uint8_t spd, uint32_t update_spd_time) {
    int8_t trn_idx = trn_hash(trn);
    uassert(trn_idx >= 0);
    train_data_pos *trn_data = &pos->data[trn_idx];

    if (trn_data->spd_init != trn_data->spd_final) {
        ULOG("***** WARNING: UNSAFE SPD PREDICTION *****\r\n");
    }

    trn_data->spd_init = trn_data->spd_final;
    trn_data->spd_final = spd;

    // first calculate distance travelled during acceleration
    trn_data->total_dist =
        get_distance_from_acceleration(&spd_data, trn,
            trn_data->spd_init, trn_data->spd_final);

    // already have an estimate to next sensor
    // must recalculate existing estimate

    // using old_estimated_time (called after state wait)

    // time spent at constant velocity
    uint32_t pre_spd_change_time = update_spd_time - trn_data->last_timestamp;
    uint32_t post_spd_change_time = trn_data->old_estimated_time - pre_spd_change_time;

    // calculate distance remaining at accel
    int32_t accel_dist = get_distance_from_velocity(&spd_data, trn,
        post_spd_change_time, trn_data->spd_init);

    // now calculate new time using calculated distance (never changes based on spd)
    uint32_t accel_time =
        estimate_initial_time_acceleration(&spd_data, trn,
            trn_data->spd_init, trn_data->spd_final, accel_dist);

    // update estimated time
    // want to overwrite estimated time
    trn_data->old_estimated_time = trn_data->old_estimated_time -
        post_spd_change_time + accel_time;

    // adjust in sensor_queue
    sensor_queue_adjust_waiting_tid(pos->sq, trn_data->last_waited_sensor_mod, trn_data->last_waited_sensor_no, trn_data->last_waited_trn_tid,
        trn_data->last_timestamp + trn_data->old_estimated_time);

    // set dist/time accelerating
    trn_data->travelled_dist = accel_dist;
    trn_data->travelled_time = accel_time;

    trn_data->queued_speed_change = NO_QUEUE;
    trn_data->queued_speed_change_time = TIME_NONE;
}

// Called by TCC when speed change is requested
void
trn_position_update_speed(trn_position *pos, uint8_t trn, uint8_t spd,
    uint32_t update_spd_time) {
    int8_t trn_idx = trn_hash(trn);
    uassert(trn_idx >= 0);
    train_data_pos *trn_data = &pos->data[trn_idx];

    if (trn_data->spd_final == spd) {
        // reaching constant spd
        if (trn_data->state == POS_STATE_HIT) {
            trn_data->queued_speed_reset = spd;
        } else {
            trn_position_end_accel(trn_data, spd);
        }

        return;
    }

    if (trn_data->spd_init == 0) {
        if (trn_data->spd_final != 0) {
            ULOG("***** WARNING: UNSAFE SPD PREDICTION *****\r\n");
        }

        trn_data->spd_final = spd;
        // first calculate distance travelled during acceleration
        trn_data->total_dist =
            get_distance_from_acceleration(&spd_data, trn,
                trn_data->spd_init, trn_data->spd_final);

        // save timestamp
        trn_data->last_timestamp = update_spd_time;
    } else {
        if (trn_data->state == POS_STATE_HIT) {
            // queue
            trn_data->queued_speed_change = spd;
            trn_data->queued_speed_change_time = update_spd_time;
        } else {
            trn_position_recalculate_starting_estimate(pos, trn, spd, update_spd_time);
        }
    }
}

static void
trn_position_make_accel_prediction(trn_position *pos, uint8_t trn) {
    train_data_pos *trn_data = &pos->data[trn_hash(trn)];

    uint32_t time_travelled_by_next_sensor =
        estimate_initial_time_acceleration(&spd_data, trn,
            trn_data->spd_init, trn_data->spd_final, trn_data->travelled_dist + trn_data->sensor_dist);

    // save old prediction value (for displaying difference)
    trn_data->old_estimated_time = trn_data->estimated_time;

    // set new prediction value
    trn_data->estimated_time = time_travelled_by_next_sensor - trn_data->travelled_time;
}

// Called by TCC when sensor is waited on by a train
int64_t
trn_position_set_sensor(trn_position *pos, uint8_t trn, int32_t dist,
    uint16_t sensor_mod, uint16_t sensor_no, uint16_t trn_tid) {
    int8_t trn_idx = trn_hash(trn);
    uassert(trn_idx >= 0);
    train_data_pos *trn_data = &pos->data[trn_idx];

    trn_data->state = POS_STATE_WAITING;
    trn_data->sensor_dist = dist; // save for later

    // save sensor mod and number for inserting/updating later
    // TCC inserts for us but we have to update it
    trn_data->last_waited_sensor_mod = sensor_mod;
    trn_data->last_waited_sensor_no = sensor_no;
    trn_data->last_waited_trn_tid = trn_tid;

    if (trn_data->spd_init == trn_data->spd_final) {
        // constant spd?
        // make prediction now
        trn_data->old_estimated_time = trn_data->estimated_time;
        trn_data->estimated_time =
            get_time_from_velocity_um(&spd_data, trn, dist, trn_data->spd_final);
    } else if (trn_data->startup) {
        // first sensor?
        // defer prediction to when we reach first sensor
        return trn_data->old_estimated_time;
    } else if (trn_data->travelled_dist + dist < trn_data->total_dist) {
        // middle sensors?
        trn_position_make_accel_prediction(pos, trn);
    } else {
        // final sensor
        // make prediction up to when we reach constant speed
        // then do usual constant speed prediction

        // non-const prediction
        int32_t accel_dist = trn_data->total_dist - trn_data->travelled_dist;
        uint32_t accel_time =
            estimate_final_time_acceleration(&spd_data, trn,
                trn_data->spd_init, trn_data->spd_final, accel_dist);

        // const prediction
        int32_t const_dist = dist - accel_dist;
        uint32_t const_time =
            get_time_from_velocity_um(&spd_data, trn, const_dist,
                trn_data->spd_final);

        // save old prediction value (for displaying difference)
        trn_data->old_estimated_time = trn_data->estimated_time;
        trn_data->estimated_time = accel_time + const_time;
    }

    if (trn_data->queued_speed_reset != NO_QUEUE) {
        trn_position_end_accel(trn_data, trn_data->queued_speed_reset);
    }

    if (trn_data->queued_speed_change != NO_QUEUE) {
        trn_position_recalculate_starting_estimate(pos, trn,
            trn_data->queued_speed_change, trn_data->queued_speed_change_time);
    }

    return trn_data->last_timestamp +
        trn_data->old_estimated_time;
}

// Called by TCC when waited on sensor is activated
void trn_position_reached_sensor(trn_position *pos, uint8_t trn,
    uint32_t activation_time) {
    int8_t trn_idx = trn_hash(trn);
    uassert(trn_idx >= 0);
    train_data_pos *trn_data = &pos->data[trn_idx];

    trn_data->state = POS_STATE_HIT;

    if (trn_data->old_estimated_time != TIME_NONE) {
        int32_t diff = (activation_time - trn_data->last_timestamp) -
            trn_data->old_estimated_time;
        update_sensor_prediction(pos->monitor_tid, trn, diff);
    }

    if (trn_data->spd_init == trn_data->spd_final) {
        // constant spd?
        trn_data->last_timestamp = activation_time;
    } else if (trn_data->startup) {
        // first sensor?
        // make first prediction (for s1) -- i.e. NEXT
        // first calculate distance to s0
        // must first calculate distance before first sensor
        uint32_t pre_sensor_time = activation_time - trn_data->last_timestamp;
        int32_t pre_sensor_dist =
            estimate_initial_distance_acceleration(&spd_data, trn,
                trn_data->spd_init, trn_data->spd_final, pre_sensor_time);

        trn_data->travelled_time = pre_sensor_time;
        trn_data->travelled_dist = pre_sensor_dist;

        // make actual prediction
        trn_position_make_accel_prediction(pos, trn);

        // adjust originally inserted sensor on wait
        sensor_queue_adjust_waiting_tid(pos->sq, trn_data->last_waited_sensor_mod, trn_data->last_waited_sensor_no, trn_data->last_waited_trn_tid,
            trn_data->last_timestamp + trn_data->estimated_time);
        // ^ NOTE: using absolute time

        trn_data->last_timestamp = activation_time;
        trn_data->startup = false;
    } else if (trn_data->travelled_dist + trn_data->sensor_dist <
        trn_data->total_dist) {
        // middle sensors?
        // increment real time and precalculated distance
        trn_data->travelled_time += (activation_time - trn_data->last_timestamp);
        trn_data->travelled_dist += trn_data->sensor_dist;

        // reset some markers
        trn_data->last_timestamp = activation_time;
        trn_data->sensor_dist = 0;
    } else {
        // final sensor
        upanic("Hit Final Sensor! Const spd not notified");
    }
}
