#include "user.h"

#include "msg.h"
#include "nameserver.h"
#include "task.h"
#include "uart-server.h"
#include "clock.h"
#include "track.h"
#include "util.h"
#include "track-data.h"
#include "speed-data.h"
#include "rpi.h"

extern track_node track[];

#define N_SWITCHES 24
#define KEY_LEFT 37
#define KEY_RIGHT 39

#define N_SENSOR 8

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

void setNextSwitch(int16_t tid, int8_t dir) {
    struct msg_us_server msg;
    msg.type = MSG_US_SENSOR_PUT;
    msg.switchDir = dir;

    Send(tid, (char *)&msg, sizeof(struct msg_us_server), NULL, 0);
}

// -------------------------------------------------------


// INIT Methods 

track_node *parseAndReply(char data, int no_of_byte) {
    int sensor_mod = (no_of_byte / 2) + 1;
    int mod_round = no_of_byte % 2;

    for (uint8_t sen_bit = 1; sen_bit <= N_SENSOR; ++sen_bit) {
        if (((data & (1 << (N_SENSOR - sen_bit))) >> (N_SENSOR - sen_bit)) == 0x01) { // bit matches?
            int16_t sensor_no = sen_bit + (mod_round * N_SENSOR);

            // return first sensor activation
            return &track[TRACK_NUM_FROM_SENSOR(sensor_mod, sensor_no)];
        }
    }
}

track_node *await_sensor_activation(int marklin, int clock, int timeout_clockTicks) {
    // Get the required tids
    uint32_t rqst_time = Time(clock);
    uint32_t end_time = rqst_time + timeout_clockTicks;

    track_node *found = NULL;

    while (rqst_time < end_time) {
        // Write sensor request
        Putc(marklin, S88_BASE + N_S88);
        
        // Get sensor data
        for (int i = 0; i < (N_S88 * 2); i++) {
            char data = Getc(marklin);

            if (data != 0x0 && found == NULL) {
                //uart_printf(CONSOLE, "received data");
                found = parseAndReply(data, i);
            }
        }

        // only return after read all sensor bytes
        if (found != NULL) {
            return found;
        }

        rqst_time = Time(clock); // estimate
    }

    return NULL;
}

static void sensor_discard_all(int marklin) {
    Putc(marklin, S88_BASE + 5);
    for (int i = 0; i < 10; ++i) Getc(marklin);
}

void init_all_trains(track_node *nodes[], int marklin, int clock) {
    speed_t speed;
    speed_t_init(&speed);

    int delays[N_TRNS] = {150, 170, 190};
    for (int i = 0; i < N_TRNS; i++) {
        uint16_t trainNo = ALL_TRNS[i];

        // discard all sensor data
        sensor_discard_all(marklin);

        // speed up train
        train_mod_speed(marklin, &speed, trainNo, 7);

        nodes[i] = await_sensor_activation(marklin, clock, 600);

        train_mod_speed(marklin, &speed, trainNo, 15);
        Delay(clock, 10);

        if (nodes[i] != NULL) {           
            train_mod_speed(marklin, &speed, trainNo, 7);
            Delay(clock, delays[i]);
        }
        train_mod_speed(marklin, &speed, trainNo, 15); 
    }
}



// ------

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
        return;
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

    WaitOutputEmpty(marklin);
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

    // Train init
    track_node *startup_pos[N_TRNS];
    init_all_trains(startup_pos, marklinTid, clockTid);

    // init all switches to loop
    startup(marklinTid);



    // infinite Loop
    for (;;) {
        Receive(&senderTid, (char *)&msg_received, sizeof(msg_us_server));

        switch (msg_received.type) {
            case MSG_US_SENSOR_PUT: {
                // Head has reached next sensor
                // -> check state of next switch
                // -> is there any ? Has it already been set
                curr_head_sensor = msg_received.node;

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
    int parent = MyParentTid();
    int console = WhoIs(CONSOLE_SERVER_NAME);
    int user = WhoIs(USER_SERVER_NAME);

    for (;;) {
        char c = Getc(console);

        if (c == KEY_LEFT || c == KEY_RIGHT) {
            setNextSwitch(user, c);
        }
    }

}

