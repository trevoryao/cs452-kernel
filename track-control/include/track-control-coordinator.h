#ifndef __TRACK_CONTROL_COORDINATOR_H__
#define __TRACK_CONTROL_COORDINATOR_H__

#include <stdbool.h>
#include <stdint.h>

#define TC_MAX_DATA_LENGTH 256
#define TC_SERVER_NAME "tc-server"

enum TC_MSG_TYPE {
    MSG_TC_PLAN_REQUEST, 
    MSG_TC_SENSOR_GET,
    MSG_TC_SENSOR_PUT, 
    MSG_TC_MAX
};

typedef struct msg_tc_server {
    enum TC_MSG_TYPE type;
    uint16_t requesterTid;
    uint64_t clockTick;
    char data[256];
} msg_tc_server;



// wrappers to be send to the server
void track_control_coordinator_main();

#endif

