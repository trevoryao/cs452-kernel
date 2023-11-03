#ifndef __TRACK_CONTROL_COORDINATOR_H__
#define __TRACK_CONTROL_COORDINATOR_H__

#include <stdbool.h>
#include <stdint.h>

#include "controller-consts.h"
#include "sensors.h"

#define TC_MAX_DATA_LENGTH 64
#define TC_SERVER_NAME "tc-server"

enum TC_MSG_TYPE {
    MSG_TC_TRAIN_PUT,
    MSG_TC_TRAIN_GET,
    MSG_TC_SWITCH_PUT,
    MSG_TC_SENSOR_GET,
    MSG_TC_SENSOR_PUT,
    MSG_TC_ERROR,
    MSG_TC_MAX
};

typedef struct msg_tc_server {
    enum TC_MSG_TYPE type;
    uint16_t requesterTid;
    uint64_t clockTick;

    union data {
        sensor sensor;      // request for sensor
        struct {
            uint16_t trn_no;
            uint16_t spd;
        } trn_cmd;

        struct {
            uint16_t sw_no;
            enum SWITCH_DIR sw_dir;
        } sw_cmd;
    } data;
} msg_tc_server;

// wrappers to be send to the server
void track_control_coordinator_main();

#endif

