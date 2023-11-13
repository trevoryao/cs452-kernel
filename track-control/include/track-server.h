#ifndef __TRACK_SERVER_H__
#define __TRACK_SERVER_H__

#include <stdbool.h>
#include <stdint.h>

#define TS_SERVER_NAME "track-server"

enum TS_MSG_TYPE {
    MSG_TS_REQUEST_SEGMENT, 
    MSG_TS_FREE_SEGMENT, 
    MSG_TS_REQUEST_SUCCESS, 
    MSG_TS_REQUEST_FAIL, 
    MSG_TS_ERROR, 
    MSG_TS_MAX
};

typedef struct msg_ts_server {
    enum TS_MSG_TYPE type;
    uint16_t requesterTid;
    uint8_t segmentID;
} msg_ts_server;


void track_server_main();

#endif
