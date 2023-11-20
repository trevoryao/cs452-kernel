#include "track-control-coordinator.h"

#include "msg.h"
#include "nameserver.h"
#include "deque.h"
#include "util.h"
#include "uart-server.h"
#include "sensor-queue.h"
#include "clock.h"
#include "task.h"
#include "sensor-worker.h"
#include "monitor.h"
#include "speed.h"
#include "track.h"
#include "uassert.h"
#include "speed-data.h"
#include "position.h"
#include "track-segment-locking.h"

#define N_SENSOR_MODULES 5
#define N_SENSORS 16
#define MAX_WAITING_TID 4
#define SENSOR_IGNORE_TIME 10

static void replyError(int senderTid) {
    struct msg_tc_server msg_reply;
    msg_reply.type = MSG_TC_ERROR;

    Reply(senderTid, (char *)&msg_reply, sizeof(struct msg_tc_server));
}

static void replyWaitingProcess(struct sensor_queue *sensor_queue, uint16_t sensor_mod, uint16_t sensor_no, uint32_t activation_ticks, trn_position *pos) {
    struct msg_tc_server msg_reply;

    int ret;
    sensor_data data;

    for (;;) {
        ret = sensor_queue_get_waiting_tid(sensor_queue, sensor_mod, sensor_no, activation_ticks, &data);

        switch (ret) {
            case SENSOR_QUEUE_DONE: return; // exit
            case SENSOR_QUEUE_TIMEOUT: {
                msg_reply.type = MSG_TC_ERROR; // let train deal
                msg_reply.requesterTid = data.tid;
                Reply(data.tid, (char *)&msg_reply, sizeof(struct msg_tc_server));

                ULOG("Dropping sensor %d,%d (trn %d)", sensor_mod, sensor_no, data.trn);

                break;
            }
            case SENSOR_QUEUE_FOUND: {
                msg_reply.type = MSG_TC_SENSOR_GET;
                msg_reply.requesterTid = data.tid;
                Reply(data.tid, (char *)&msg_reply, sizeof(struct msg_tc_server));

                if (data.pos_rqst) {
                    trn_position_reached_sensor(pos, data.trn, activation_ticks);
                }

                // ULOG("Replying to tid %d for sensor mod %d and sensor no %d (trn %d)\r\n", data.tid, sensor_mod, sensor_no, data.trn);

                break;
            }
            default: upanic("Unknown result from sensor_queue_get %d\r\n", ret);
        }
    }
}

static void replyTrainSpeed(struct speed_t *spd_t, uint16_t trainNo, uint16_t senderTid) {
    struct msg_tc_server msg_reply;
    msg_reply.type = MSG_TC_TRAIN_GET;

    msg_reply.data.trn_cmd.trn_no = trainNo;
    msg_reply.data.trn_cmd.spd = speed_get(spd_t, trainNo);

    Reply(senderTid, (char *)&msg_reply, sizeof(struct msg_tc_server));
}

static void replySwitchPosition(uint16_t senderTid, uint16_t sw_no, enum SWITCH_DIR sw_dir) {
    struct msg_tc_server msg_reply;
    msg_reply.type = MSG_TC_SENSOR_PUT;

    msg_reply.data.sw_cmd.sw_no = sw_no;
    msg_reply.data.sw_cmd.sw_dir = sw_dir;

    Reply(senderTid, (char *)&msg_reply, sizeof(struct msg_tc_server));
}

void track_control_coordinator_main() {
    // structs for reading & writing
    int senderTid;
    struct msg_tc_server msg_received;

    // Data structure to keep latest sensor activations
    struct sensor_queue sensor_queue;
    sensor_queue_init(&sensor_queue);

    uint16_t latestSensorMod = 0;
    uint16_t latestSensorNo = 0;
    uint16_t latestSensorTimestamp = 0;

    // Structure for UI sensor
    struct deque ui_sensor_queue;
    deque_init(&ui_sensor_queue, 4);

    // Data structure for registered trains
    trn_data registered_trns[N_TRNS];
    memset(registered_trns, 0, N_TRNS * sizeof(trn_data));

    // Register at Nameserver
    RegisterAs(TC_SERVER_NAME);

    int clockTid = WhoIs(CLOCK_SERVER_NAME);
    int marklinTid = WhoIs(MARKLIN_SERVER_NAME);
    int consoleTid = WhoIs(CONSOLE_SERVER_NAME);
    int tsTid = WhoIs(TS_SERVER_NAME);

    // start up a sensorWorker
    Create(P_HIGH, sensor_worker_main);

    /*
    *
    *   Structures to save track & train state
    *
    */
    struct speed_t spd_t;
    speed_t_init(&spd_t);

    struct speed_data spd_data;
    speed_data_init(&spd_data);

    // position algorithm
    trn_position pos;
    trn_position_init(&pos, &sensor_queue);

    for (;;) {
        Receive(&senderTid, (char *)&msg_received, sizeof(struct msg_tc_server));

        switch (msg_received.type) {
            case MSG_TC_TRAIN_REGISTER: {
                int8_t trn_idx = trn_hash(msg_received.data.trn_register.trn_no);

                if (trn_idx < 0 || registered_trns[trn_idx].tid > 0) {
                    replyError(senderTid);
                } else {
                    memcpy(&registered_trns[trn_idx], &msg_received.data.trn_register, sizeof(trn_data));
                    Reply(senderTid, (char *)&msg_received, sizeof(msg_tc_server)); // echo
                }

                break;
            }
            case MSG_TC_TRAIN_DONE: {
                int8_t trn_idx = trn_hash(msg_received.data.trn_register.trn_no);

                if (trn_idx < 0 || registered_trns[trn_idx].tid == 0) {
                    replyError(senderTid);
                } else {
                    memset(&registered_trns[trn_idx], 0, sizeof(trn_data));
                    reset_tc_params(consoleTid, msg_received.data.trn_register.trn_no); // clear console
                    Reply(senderTid, (char *)&msg_received, sizeof(msg_tc_server)); // echo
                }

                sensor_queue_free_train(&sensor_queue,
                    msg_received.data.trn_register.trn_no);
                trn_position_reset(&pos,
                    msg_received.data.trn_register.trn_no);

                track_server_free_all(tsTid, msg_received.data.trn_register.trn_no);
            
                break;
            }
            case MSG_TC_TRAIN_GET: {
                replyTrainSpeed(&spd_t, msg_received.data.trn_cmd.trn_no, senderTid);
                break;
            }
            case MSG_TC_SENSOR_GET: {
                // enqueue to sensor
                sensor received_sensor = msg_received.data.sensor;

                int64_t expected_time = TIME_NONE;
                if (msg_received.positionRequest) {
                    //uart_printf(CONSOLE, "Got position request for train %d", msg_received.trainNo);
                    // save trainNo
                    expected_time = trn_position_set_sensor(&pos, msg_received.trainNo, msg_received.distance_to_next_sensor,
                        received_sensor.mod_sensor, received_sensor.mod_num,
                        msg_received.requesterTid);
                }

                sensor_data data = {
                    msg_received.requesterTid,
                    msg_received.trainNo,
                    expected_time,
                    msg_received.positionRequest,
                };

                sensor_queue_add_waiting_tid(&sensor_queue, received_sensor.mod_sensor, received_sensor.mod_num, &data);
                break;
            }
            case MSG_TC_SENSOR_PUT: {
                Reply(senderTid, NULL, 0);

                uint16_t sensor_module_number = msg_received.data.sensor.mod_num;
                uint16_t sensor_number = msg_received.data.sensor.mod_sensor;
                int current_Timestamp = msg_received.clockTick;

                if (latestSensorMod == sensor_module_number && latestSensorNo == sensor_number
                    && ((latestSensorTimestamp + SENSOR_IGNORE_TIME) > current_Timestamp)) {
                    // ignore sensor message
                    break;
                } else {
                    latestSensorMod = sensor_module_number;
                    latestSensorNo = sensor_number;
                    latestSensorTimestamp = current_Timestamp;

                    // dequeue waiting processes
                    replyWaitingProcess(&sensor_queue, sensor_module_number, sensor_number, current_Timestamp, &pos);

                    // inform UI about sensor update
                    update_triggered_sensor(consoleTid, &ui_sensor_queue, (sensor_module_number - 1), sensor_number);
                }

                break;
            }
            case MSG_TC_SWITCH_PUT: {
                switch_throw(marklinTid, msg_received.data.sw_cmd.sw_no, msg_received.data.sw_cmd.sw_dir);
                replySwitchPosition(senderTid, msg_received.data.sw_cmd.sw_no, msg_received.data.sw_cmd.sw_dir);
                update_switch(consoleTid, msg_received.data.sw_cmd.sw_no, msg_received.data.sw_cmd.sw_dir);
                break;
            }
            case MSG_TC_TRAIN_PUT: {
                train_mod_speed(marklinTid, &spd_t, msg_received.data.trn_cmd.trn_no, msg_received.data.trn_cmd.spd);
                uint32_t speed_change_time = Time(clockTid);
                replyTrainSpeed(&spd_t, msg_received.data.trn_cmd.trn_no, senderTid);
                update_speed(consoleTid, &spd_t, msg_received.data.trn_cmd.trn_no);

                trn_position_update_speed(&pos, msg_received.data.trn_cmd.trn_no, msg_received.data.trn_cmd.spd, speed_change_time);
                break;
            }
            default: upanic("Unknown TCC msg type: %d", msg_received.type);
        }
    }
}
