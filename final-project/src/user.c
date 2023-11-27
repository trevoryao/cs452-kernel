#include "user.h"

#include "msg.h"
#include "nameserver.h"
#include "task.h"
#include "uart-server.h"
#include "clock.h"
#include "track.h"
#include "util.h"

extern *track[];
#define N_SWITCHES 24

// -------------------------------------------------------
// WRAPPER Methods
track_node *user_reached_sensor(int16_t tid, track_node *track) {
    struct msg_us_server msg, reply;
    msg.type = MSG_US_SENSOR_PUT;
    msg.node = track;

    int ret = Send(tid, (char *)&msg, sizeof(struct msg_us_server), (char *)&reply, sizeof(struct msg_us_server));

    if (ret < 0 || reply.type == MSG_US_ERROR) {
        return NULL;
    } else {
        return reply.node;
    }
}

void user_display_distance(int16_t tid, int32_t um) {
    struct msg_us_server msg;
    msg.type = MSG_US_DISPLAY_DISTANCE;
    msg.distance = um;

    Send(tid, (char *)&msg, sizeof(struct msg_us_server), NULL, 0);
}

track_node *user_get_next_sensor(int16_t tid) {
    struct msg_us_server msg, reply;
    msg.type = MSG_US_GET_NEXT_SENSOR;

     
    int ret = Send(tid, (char *)&msg, sizeof(struct msg_us_server), (char *)&reply, sizeof(struct msg_us_server));

    if (ret < 0 || reply.type == MSG_US_ERROR) {
        return NULL;
    } else {
        return reply.node;
    }
}

// -------------------------------------------------------


// make sure to only pass a valid switch
int8_t get_switch_num(track_node *node) {
    if (node->num <= 18) {
        return node->num;
    } else { 
        return node->num - 134;
    }
}

enum SWITCH_DIR get_switch_dir(enum SWITCH_DIR *switch_pos, track_node *node) {
    if (node->type != NODE_BRANCH) {
        return UNKNOWN;
    }
    int8_t num = get_switch_num(node);

    return switch_pos[num];

}

void save_switch_dir(enum SWITCH_DIR *switch_pos, track_node *node, enum SWITCH_DIR switch_dir) {
    if (node->type != NODE_BRANCH) {
        return UNKNOWN;
    }
    int8_t num = get_switch_num(node);

    switch_pos[num] = switch_dir;
}



void startup(int marklin) {
    // set all switches to a loop

    // AHEAD switches
    switch_throw(marklin, 7, STRT);
    switch_throw(marklin, 14, STRT);
    switch_throw(marklin, 8, STRT);

    // reverse direction switches
    switch_throw(marklin, 6, STRT);
    switch_throw(marklin, 15, STRT);
    switch_throw(marklin, 11, CRV);
    switch_throw(marklin, 9, STRT);

}

track_node *next_sensor(track_node *curr) {
    if (curr->type != NODE_SENSOR) {
        return NULL;
    }

    track_node *next = curr->edge[DIR_AHEAD].dest;
    // go through the data and return the next one 
    while(next->type != NODE_SENSOR) {
        if (next->type == NODE_BRANCH) {
            next = next->edge[DIR_STRAIGHT].dest;
        } else {
            next = next->edge[DIR_AHEAD].dest;
        }
    }
    return next;
}

void replyError(int tid) {
    struct msg_us_server msg_reply;
    msg_reply.type = MSG_US_ERROR;

    Reply(tid, (char *)&msg_reply, sizeof(struct msg_us_server));
}

void replySensor(int tid, track_node *node) { 
    struct msg_us_server msg_reply;
    msg_reply.type = MSG_US_GET_NEXT_SENSOR;
    msg_reply.node = node;

    Reply(tid, (char *)&msg_reply, sizeof(struct msg_us_server));
}

// Tasks for the user main
/*
*   - Switch Management:
*       - Figure out what the next switch is
*       - Handle user input through notifier
*       - Have some sort of timeout to set switch to straight if not set by user
*   - Sensor management:
*       - based on switch position -> what is next sensor
*   - Monitor:
*       - display required data
*/
void user_server_main(void) {
    // structs for reading & writing 
    int senderTid;
    struct msg_us_server msg_received;
    
    // register and get required tids
    RegisterAs(USER_SERVER_NAME);
    int clockTid = WhoIs(CLOCK_SERVER_NAME);
    int marklinTid = WhoIs(MARKLIN_SERVER_NAME);
    int consoleTid = WhoIs(CONSOLE_SERVER_NAME);

    // TODO: Register notifiers

    // last sensor
    track_node *curr_head_sensor;
    track_node *next_expected_sensor;
    

    enum SWITCH_DIR switch_pos[N_SWITCHES];
    memset(&switch_pos, UNKNOWN, N_SWITCHES);

    track_node *last_switch = NULL;
    track_node *next_switch = NULL;
    enum SWITCH_DIR last_sw_di = UNKNOWN;
    enum SWITCH_DIR next_sw_di = UNKNOWN;


    // init track 
    init_track(marklinTid);
    startup(marklinTid);

    // infinite Loop
    for (;;) {
        Receive(&senderTid, (char *)&msg_received, sizeof(msg_us_server));

        switch (msg_received.type) {
            case MSG_US_SENSOR_PUT: {
                // Head has reached next sensor
                // -> check state of next switch 
                // -> is there any ? Has it already been set
                curr_head_sensor = next_expected_sensor;

                // TODO: compute next sensor
                next_expected_sensor = next_sensor(curr_head_sensor);

                replySensor(senderTid, next_expected_sensor);
                break;
            }

            case MSG_US_DISPLAY_DISTANCE: {
                // TODO:
                replyError(senderTid);
                break;
            }

            case MSG_US_GET_NEXT_SENSOR: {
                // return the next sensor on the route
                replySensor(senderTid, next_expected_sensor);
                break;
            }

            case MSG_US_SET_SWITCH: {
                // check if it is allowed to throw next switch
                // maybe lock it?
                replyError(senderTid);
                break;
            }
            
        
            default:
                replyError(senderTid);
                break;
        }
    }
}


void user_input_notifier(void) {


}

