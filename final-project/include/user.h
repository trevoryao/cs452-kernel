#ifndef __USER_H__
#define __USER_H__

#include <stdint.h>
#include "track-node.h"
#include "controller-consts.h"

#define USER_SERVER_NAME "user_server"
#define LEFT 1
#define RIGHT 0

enum US_MSG_TYPE {
    MSG_US_SENSOR_PUT,
    MSG_US_DISPLAY_DISTANCE,
    MSG_US_GET_NEXT_SENSOR,
    MSG_US_SET_SWITCH,
    MSG_US_UPDATE_SPEED,
    MSG_US_OK,
    MSG_US_ERROR,
    MSG_US_QUIT,
    MSG_US_MAX
};


typedef struct switch_data {
    uint8_t left_is_straight;
    uint8_t allowedToBeSet;
    uint8_t user_set;
    enum SWITCH_DIR curr_dir;

} switch_data;

typedef struct msg_us_server {
    enum US_MSG_TYPE type;
    int32_t distance;
    track_node *node;
    uint8_t trainNo;
    uint8_t speed;
    int8_t switchDir;
    uint8_t reverse;
} msg_us_server;

void user_server_main(void);
void user_input_notifier(void);

void user_display_distance(int16_t tid, int32_t um);
track_node *user_get_next_sensor(int16_t tid);
track_node *user_reached_sensor(int16_t tid, track_node *track, uint8_t *trn, int32_t *dist, uint8_t *reverse); // return train to start up, if any

void user_updated_head_speed(int16_t tid, uint8_t trainNo, uint8_t speed);

#endif
