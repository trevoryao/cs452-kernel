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
#include "rpi.h"
#include "speed-data.h"

#define N_SENSOR_MODULES 5
#define N_SENSORS 16
#define MAX_WAITING_TID 4
#define SENSOR_IGNORE_TIME 10

void replyError(int senderTid) {
    struct msg_tc_server msg_reply;
    msg_reply.type = MSG_TC_ERROR;

    Reply(senderTid, (char *)&msg_reply, sizeof(struct msg_tc_server));
}

void replyWaitingProcess(struct sensor_queue *sensor_queue, uint16_t sensor_mod, uint16_t sensor_no) {
    struct msg_tc_server msg_reply;
    msg_reply.type = MSG_TC_SENSOR_GET;

    while(!sensor_queue_empty(sensor_queue, sensor_mod, sensor_no)) {
        uint16_t tid = sensor_queue_get_waiting_tid(sensor_queue, sensor_mod, sensor_no);
        msg_reply.requesterTid = tid;
        Reply(tid, (char *)&msg_reply, sizeof(struct msg_tc_server));
        uart_printf(CONSOLE, "Replying to tid %d for sensor mod %d and sensor no %d\r\n", tid, sensor_mod, sensor_no);
    }
}

void replyTrainSpeed(struct speed_t *spd_t, uint16_t trainNo, uint16_t senderTid) {
    struct msg_tc_server msg_reply;
    msg_reply.type = MSG_TC_TRAIN_GET;

    msg_reply.data.trn_cmd.trn_no = trainNo;
    msg_reply.data.trn_cmd.spd = speed_get(spd_t, trainNo);

    Reply(senderTid, (char *)&msg_reply, sizeof(struct msg_tc_server));
}

void replySwitchPosition(uint16_t senderTid, uint16_t sw_no, enum SWITCH_DIR sw_dir) {
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

    // start up a sensorWorker
    int sensorWorker = Create(P_HIGH, sensor_worker_main);

    /*
    *
    *   Structures to save track & train state
    *
    */
    struct speed_t spd_t;
    speed_t_init(&spd_t);

    struct speed_data spd_data;
    speed_data_init(&spd_data);

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
                    Reply(senderTid, (char *)&msg_received, sizeof(msg_tc_server)); // echo
                }

                break;
            }

            case MSG_TC_TRAIN_GET: {
                /* code */
                replyTrainSpeed(&spd_t, msg_received.data.trn_cmd.trn_no, senderTid);
                break;
            }

            case MSG_TC_SENSOR_GET: {
                // enqueue to sensor
                // TODO: fix when better structure
                sensor received_sensor = msg_received.data.sensor;
                sensor_queue_add_waiting_tid(&sensor_queue, received_sensor.mod_num, received_sensor.mod_sensor, msg_received.requesterTid);
                uart_printf(CONSOLE, "Enqueueing tid %d for sensor mod %d and sensor no %d\r\n", msg_received.requesterTid, received_sensor.mod_num, received_sensor.mod_sensor);
                break;
            }

            case MSG_TC_SENSOR_PUT: {
                Reply(senderTid, NULL, 0);

                uint16_t sensor_mod = msg_received.data.sensor.mod_sensor;
                uint16_t sensor_no = msg_received.data.sensor.mod_num;
                int current_Timestamp = Time(clockTid);

                if (latestSensorMod == sensor_mod && latestSensorNo == sensor_no
                    && ((latestSensorTimestamp + SENSOR_IGNORE_TIME) > current_Timestamp)) {
                    // ignore sensor message
                    break;
                } else {
                    latestSensorMod = sensor_mod;
                    latestSensorNo = sensor_no;
                    latestSensorTimestamp = current_Timestamp;

                    // dequeue waiting processes
                    replyWaitingProcess(&sensor_queue, sensor_mod, sensor_no);

                    // inform UI about sensor update
                    update_triggered_sensor(consoleTid, &ui_sensor_queue, sensor_mod, sensor_no);

                }

                break;
            }

            case MSG_TC_SWITCH_PUT: {
                /* code */
                // set train speed
                switch_throw(marklinTid, msg_received.data.sw_cmd.sw_no, msg_received.data.sw_cmd.sw_dir);
                replySwitchPosition(senderTid, msg_received.data.sw_cmd.sw_no, msg_received.data.sw_cmd.sw_dir);
                update_switch(consoleTid, msg_received.data.sw_cmd.sw_no, msg_received.data.sw_cmd.sw_dir);
                break;
            }

            case MSG_TC_TRAIN_PUT: {
                /* code */
                train_mod_speed(marklinTid, &spd_t, msg_received.data.trn_cmd.trn_no, msg_received.data.trn_cmd.spd);
                replyTrainSpeed(&spd_t, msg_received.data.trn_cmd.trn_no, senderTid);
                update_speed(consoleTid, &spd_t, msg_received.data.trn_cmd.trn_no);
                break;
            }

            default:
                break;
        }
    }


}