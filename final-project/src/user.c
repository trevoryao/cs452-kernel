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
#include "task.h"

#include "control-msgs.h"
#include "uassert.h"

extern track_node track[];

#define N_SWITCHES 23
#define KEY_LEFT 37
#define KEY_RIGHT 39

#define N_SENSOR 8
#define THREE_WAY_OFFSET 134

// -------------------------------------------------------
// WRAPPER Methods
track_node *user_reached_sensor(int16_t tid, track_node *track, uint8_t *trn, int32_t *dist) {
    struct msg_us_server msg, reply;
    msg.type = MSG_US_SENSOR_PUT;
    msg.node = track;
    msg.distance = dist;
    msg.trainNo = trn;
    uart_printf(CONSOLE, "sending user_reached \r\n");
    int ret = Send(tid, (char *)&msg, sizeof(struct msg_us_server), (char *)&reply, sizeof(struct msg_us_server));

    if (ret < 0 || reply.type == MSG_US_ERROR) {
        return NULL;
    } else {
        return reply.node;
    }
}

void user_display_distance(int16_t tid, int32_t *um) {
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
    msg.type = MSG_US_SET_SWITCH;
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
    return NULL;
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
        return node->num - THREE_WAY_OFFSET;
    }
}

enum SWITCH_DIR get_switch_dir(switch_data *switches, track_node *node) {
    if (node->type != NODE_BRANCH) {
        return UNKNOWN;
    }
    int8_t num = get_switch_num(node);

    return switches[num].curr_dir;

}

enum SWITCH_DIR set_switch_dir(switch_data *switches, int marklin, track_node *node, uint8_t left) {
    if (node->type != NODE_BRANCH) {
        return UNKNOWN;
    }
    int8_t num = get_switch_num(node);

    // TODO protection if not allowed
    if (!switches[num].allowedToBeSet) {
        // just return if not allowed to be set
        return switches[num].curr_dir;
    }

    // TODO -> special case for 3-way switches
    if (num > 18)  {
        int index1 = (num <= 20) ? 19 : 21;
        int index2 = (num <= 20) ? 20 : 22;

        if (left) {
            switches[index1].curr_dir = STRT;
            switches[index2].curr_dir = CRV;
        } else {
            switches[index1].curr_dir = CRV;
            switches[index2].curr_dir = STRT;
        }
        switch_throw(marklin, index1 + THREE_WAY_OFFSET, switches[index1].curr_dir);
        switch_throw(marklin, index2 + THREE_WAY_OFFSET, switches[index2].curr_dir);
        return switches[num].curr_dir;
    }

    switches[num].curr_dir = switches[num].left_is_straight ? (left ? STRT : CRV) : (left ? CRV : STRT);

    switch_throw(marklin, num, switches[num].curr_dir);

    return switches[num].curr_dir;
}

void startup(int marklin) {
    // set all switches to a loop

    // AHEAD switches
    switch_throw(marklin, 7, STRT);
    switch_throw(marklin, 14, STRT);
    switch_throw(marklin, 8, STRT);

    // set the switches, which are not allowed to be set
    switch_throw(marklin, 1, STRT);
    switch_throw(marklin, 2, STRT);
    switch_throw(marklin, 4, STRT);
    switch_throw(marklin, 3, CRV);
    switch_throw(marklin, 12, CRV);

    WaitOutputEmpty(marklin);
}

void init_switch_data(switch_data *data) {
    data[1].allowedToBeSet = false;
    data[1].curr_dir = STRT;

    data[2].allowedToBeSet = false;
    data[2].curr_dir = STRT;

    data[3].allowedToBeSet = false;
    data[3].curr_dir = CRV;

    data[4].allowedToBeSet = false;
    data[4].curr_dir = STRT;

    data[12].allowedToBeSet = false;
    data[12].curr_dir = CRV;

    data[5].allowedToBeSet = true;
    data[5].curr_dir = UNKNOWN;
    data[5].left_is_straight = 0;

    data[6].allowedToBeSet = true;
    data[6].curr_dir = UNKNOWN;
    data[6].left_is_straight = 1;

    data[7].allowedToBeSet = true;
    data[7].curr_dir = STRT;
    data[7].left_is_straight = 0;

    data[8].allowedToBeSet = true;
    data[8].curr_dir = STRT;
    data[8].left_is_straight = 1;

    data[9].allowedToBeSet = true;
    data[9].curr_dir = UNKNOWN;
    data[9].left_is_straight = 0;

    data[10].allowedToBeSet = true;
    data[10].curr_dir = UNKNOWN;
    data[10].left_is_straight = 0;

    data[11].allowedToBeSet = true;
    data[11].curr_dir = UNKNOWN;
    data[11].left_is_straight = 0;

    data[13].allowedToBeSet = true;
    data[13].curr_dir = UNKNOWN;
    data[13].left_is_straight = 1;

    data[14].allowedToBeSet = true;
    data[14].curr_dir = STRT;
    data[14].left_is_straight = 1;

    data[15].allowedToBeSet = true;
    data[15].curr_dir = UNKNOWN;
    data[15].left_is_straight = 0;

    data[16].allowedToBeSet = true;
    data[16].curr_dir = UNKNOWN;
    data[16].left_is_straight = 0;

    data[17].allowedToBeSet = true;
    data[17].curr_dir = UNKNOWN;
    data[17].left_is_straight = 1;

    data[18].allowedToBeSet = true;
    data[18].curr_dir = UNKNOWN;
    data[18].left_is_straight = 0;

    data[19].allowedToBeSet = true;
    data[19].curr_dir = UNKNOWN;
    data[19].left_is_straight = 1;

    data[20].allowedToBeSet = true;
    data[20].curr_dir = UNKNOWN;
    data[20].left_is_straight = 0;

    data[21].allowedToBeSet = true;
    data[21].curr_dir = UNKNOWN;
    data[21].left_is_straight = 1;

    data[22].allowedToBeSet = true;
    data[22].curr_dir = UNKNOWN;
    data[22].left_is_straight = 0;
}

track_node *next_sensor(track_node *curr, switch_data *data, int32_t *distance) {
    if (curr->type != NODE_SENSOR) {
        return NULL;
    }

    *distance = 0;
    
    *distance += curr->edge[DIR_AHEAD].dist;
    track_node *next = curr->edge[DIR_AHEAD].dest;
    // go through the data and return the next one
    while(next->type != NODE_SENSOR) {
        if (next->type == NODE_BRANCH) {
            enum SWITCH_DIR dir = get_switch_dir(data, next);

            // unknown switch dir should not happen -> should be set ahead of time
            uassert(dir != UNKNOWN);

            int dirIndex = (dir == STRT) ? DIR_STRAIGHT : DIR_CURVED;

            *distance += next->edge[dirIndex].dist;
            next = next->edge[dirIndex].dest;
        } else {
            *distance += next->edge[DIR_AHEAD].dist;
            next = next->edge[DIR_AHEAD].dest;
        }
    }
    return next;
}

track_node *get_next_switch(track_node *next) {
    while(next->type != NODE_BRANCH) {
        next = next->edge[DIR_AHEAD].dest;
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
    track_node *curr_head_sensor = NULL;
    track_node *next_expected_sensor = NULL;


    switch_data switches[N_SWITCHES];
    memset(&switches, 0, N_SWITCHES * sizeof(switch_data));

    track_node *next_switch = NULL;

    // Train init
    track_node *startup_pos[N_TRNS];
    //init_all_trains(startup_pos, marklinTid, clockTid);

    // init all switches to loop
    init_switch_data(switches);
    startup(marklinTid);
    
    Create(P_HIGH, user_input_notifier);


    SendParentReady();

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
                next_expected_sensor = next_sensor(curr_head_sensor, switches, msg_received.distance);

                // compute next switch
                next_switch = get_next_switch(next_expected_sensor);
                uart_printf(CONSOLE, "next sensor %d and next switch %d\r\n", next_expected_sensor->num, next_switch->num);

                // check if next expected sensor is in startup
                for (int i = 0; i < N_TRNS; i++) {
                    if (startup_pos[i] != NULL && startup_pos[i] == next_expected_sensor) {
                        startup_pos[i] = NULL;
                        *msg_received.trainNo = ALL_TRNS[i];
                        break;
                    }
                }

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
                if (next_switch == NULL) {
                    uart_printf(CONSOLE, "no next switch yet\r\n");
                } else {
                    set_switch_dir(switches, marklinTid, next_switch, msg_received.switchDir);
                }
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
    int console = WhoIs(CONSOLE_SERVER_NAME);
    int user = WhoIs(USER_SERVER_NAME);

    int i = 0;

    for (;;) {
        char c = Getc(console);

        if (i == 0 && c == 27) {
            i = 1;
        } else if (i == 1 && c == 91) {
            i = 2;
        } else if (i == 2 && c == 67) {
            // valid right arrow key
            setNextSwitch(user, RIGHT);
        } else if (i == 2 && c == 68) {
            // valid left arrow key
            setNextSwitch(user, LEFT);
        } else {
            // invalid sequence
            i = 0;
        }
    }

}

