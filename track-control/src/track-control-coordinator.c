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

#define N_SENSOR_MODULES 5
#define N_SENSORS 16
#define MAX_WAITING_TID 4
#define SENSOR_IGNORE_TIME 10

void replyWaitingProcess(struct sensor_queue *sensor_queue, uint16_t sensor_mod, uint16_t sensor_no) {
    struct msg_tc_server msg_reply;
    msg_reply.type = MSG_TC_SENSOR_GET;

    while(!sensor_queue_empty(sensor_queue, sensor_mod, sensor_no)) {
        uint16_t tid = sensor_queue_get_waiting_tid(sensor_queue, sensor_mod, sensor_no);
        msg_reply.requesterTid = tid;
        Reply(tid, (char *)&msg_reply, sizeof(struct msg_tc_server));
    }
} 

void track_control_coordinator_main() {
    // structs for reading & writing
    int senderTid;
    struct msg_tc_server msg_received, msg_send;


    // Data structure to keep latest sensor activations
    struct sensor_queue sensor_queue;
    sensor_queue_init(&sensor_queue);
    uint16_t latestSensorMod = 0;
    uint16_t latestSensorNo = 0;
    uint16_t latestSensorTimestamp = 0;

    // Structure for UI sensor
    struct deque ui_sensor_queue;
    deque_init(&ui_sensor_queue, 3);

    // Register at Nameserver
    RegisterAs(TC_SERVER_NAME);

    int clockTid = WhoIs(CLOCK_SERVER_NAME);
    int marklinTid = WhoIs(MARKLIN_SERVER_NAME);
    int consoleTid = WhoIs(CONSOLE_SERVER_NAME);

    // start up a sensorWorker
    int sensorWorker = Create(P_NOTIF, sensor_worker_main);

    for (;;) {
        Receive(&senderTid, (char *)&msg_received, sizeof(struct msg_tc_server));


        switch (msg_received.type) {
            case MSG_TC_PLAN_REQUEST: {
                /* code */
                break;
            }


            case MSG_TC_SENSOR_GET: {
                // enqueue to sensor
                // TODO: fix when better structure
                uint16_t sensor_mod = msg_received.data[0];
                uint16_t sensor_no = msg_received.data[1];
                sensor_queue_add_waiting_tid(&sensor_queue, sensor_mod, sensor_no, msg_received.requesterTid);
                break;
            }

            case MSG_TC_SENSOR_PUT: {
                Reply(senderTid, NULL, 0);
                // TODO:
                // for (..)
                //      if timestamp > sensorTimestamp[..][..] {add sensor}

                // TODO: Maybe replace
                uint16_t sensor_mod = msg_received.data[0];
                uint16_t sensor_no = msg_received.data[1];
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

            default:
                break;
        }
    }


}