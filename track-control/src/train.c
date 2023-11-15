#include "train.h"

#include "clock.h"
#include "msg.h"
#include "nameserver.h"
#include "task.h"
#include "uassert.h"
#include "uart-server.h"
#include "util.h"

#include "monitor.h"
#include "routing.h"
#include "track-control.h"
#include "track-control-coordinator.h"
#include "track-data.h"

extern track_node track[];

enum TRAIN_MSG_TYPE {
    MSG_TRAIN_PARAMS,
    MSG_TRAIN_ACTION,
    MSG_TRAIN_NOTIFY_ROUTE,
    MSG_TRAIN_NOTIFY_SPD,
    MSG_TRAIN_QUIT,
    MSG_TRAIN_ACK,
    MSG_TRAIN_ERROR,
    MSG_TRAIN_MAX
};

typedef struct train_params {
    uint8_t trn;
    enum SPEEDS spd;
    track_node *start, *end;
    int32_t offset;
} train_params;

typedef struct route_notif_action {
    int16_t sensor_num; // could be SENSOR_NONE
    int32_t sensor_dist; // only valid if sensor_num != SENSOR_NONE
    uint32_t delay; // ticks (could be 0)
    uint8_t trn;
} route_notif_action;

typedef struct spd_notif_action {
    int16_t sensor_num; // could be SENSOR_NONE
    int32_t sensor_dist; // only valid if sensor_num != SENSOR_NONE
    uint32_t delay; // ticks (could be 0)
    uint8_t trn;

    routing_action routing_action;
} spd_notif_action;

typedef struct train_msg {
    enum TRAIN_MSG_TYPE type;

    union {
        train_params params;
        // a request is either a sensor or a timestamp or both (but never neither)
        route_notif_action route_action;
        spd_notif_action spd_action;
    } payload;
} train_msg;

static void do_action(int tc_tid, uint8_t trn, routing_action *action) {
    switch (action->action_type) {
        case SWITCH:
            // ULOG("[train] Switch %d to %c\r\n",
            //     action->action.sw.num, (action->action.sw.dir == STRT) ? 'S' : 'C');
            track_control_set_switch(tc_tid, action->action.sw.num, action->action.sw.dir);
            break;
        case SPD_CHANGE:
        case SPD_REACHED:
            // ULOG("[train] Set Speed to %d\r\n", action->action.spd);
            track_control_set_train_speed(tc_tid, trn, action->action.spd);
            break;
        default:
            // ULOG("[train] no action (%d)\r\n", action->action_type);
            break; // none needed for any other actions
    }
}

#ifdef USERLOG
#define NOTIF_TYPE(type) ((type == 2) ? "route" : "spd")
#endif

static void train_speed_notifier(void) {
    int ptid;
    train_msg msg;

    enum TRAIN_MSG_TYPE type = MSG_TRAIN_NOTIFY_SPD;

    int tc_server_tid = WhoIs(TC_SERVER_NAME);
    int clock_tid = WhoIs(CLOCK_SERVER_NAME);

    spd_notif_action action;

    for (;;) {
        uassert(Receive(&ptid, (char *)&msg, sizeof(train_msg)) == sizeof(train_msg));

        enum TRAIN_MSG_TYPE rcvd_type = msg.type;

        switch (rcvd_type) {
            case MSG_TRAIN_ACTION:
                memcpy(&action, &msg.payload.spd_action, sizeof(spd_notif_action));
                // fallthrough
            case MSG_TRAIN_QUIT:
                // valid
                msg.type = MSG_TRAIN_ACK;
                break;
            default:
                // invalid
                msg.type = MSG_TRAIN_ERROR;
                break;
        }

        Reply(ptid, (char *)&msg, sizeof(train_msg)); // ACK/ERROR

        if (rcvd_type == MSG_TRAIN_QUIT) break; // EXIT: quit

        // wait for sensor activation
        if (action.sensor_num != SENSOR_NONE) {
            // ULOG("[train-notifier-%s] wait on sensor %c%d\r\n",
            //     NOTIF_TYPE(type),
            //     SENSOR_MOD(action.sensor_num) - 1 + 'A',
            //     SENSOR_NO(action.sensor_num));
            // Delay(clock_tid, 10);
            track_control_wait_sensor(tc_server_tid,
                SENSOR_MOD(action.sensor_num),
                SENSOR_NO(action.sensor_num),
                action.sensor_dist,
                action.trn, false); // don't update position for speed
        }

        if (action.delay != 0) {
            // ULOG("[train-notifier %s] delay %d ms\r\n",
            //     NOTIF_TYPE(type), action.delay * 10);
            Delay(clock_tid, action.delay);
        }

        // perform action immediately
        do_action(tc_server_tid, action.trn, &action.routing_action);

        // finished, let server perform action
        msg.type = type;
        Send(ptid, (char *)&msg, sizeof(train_msg), NULL, 0);
    }
}

static void train_route_notifier(void) {
    int ptid;
    train_msg msg;

    enum TRAIN_MSG_TYPE type = MSG_TRAIN_NOTIFY_ROUTE;

    int tc_server_tid = WhoIs(TC_SERVER_NAME);
    int clock_tid = WhoIs(CLOCK_SERVER_NAME);

    route_notif_action action;

    for (;;) {
        uassert(Receive(&ptid, (char *)&msg, sizeof(train_msg)) == sizeof(train_msg));

        enum TRAIN_MSG_TYPE rcvd_type = msg.type;

        switch (rcvd_type) {
            case MSG_TRAIN_ACTION:
                memcpy(&action, &msg.payload.route_action, sizeof(route_notif_action));
                // fallthrough
            case MSG_TRAIN_QUIT:
                // valid
                msg.type = MSG_TRAIN_ACK;
                break;
            default:
                // invalid
                msg.type = MSG_TRAIN_ERROR;
                break;
        }

        Reply(ptid, (char *)&msg, sizeof(train_msg)); // ACK/ERROR

        if (rcvd_type == MSG_TRAIN_QUIT) break; // EXIT: quit

        // uint32_t delay_until_ticks = Time(clock_tid) + action.delay;

        // wait for sensor activation
        if (action.sensor_num != SENSOR_NONE) {
            // ULOG("[train-notifier-%s] wait on sensor %c%d\r\n",
            //     NOTIF_TYPE(type),
            //     SENSOR_MOD(action.sensor_num) - 1 + 'A',
            //     SENSOR_NO(action.sensor_num));
            // Delay(clock_tid, 10);
            track_control_wait_sensor(tc_server_tid,
                SENSOR_MOD(action.sensor_num),
                SENSOR_NO(action.sensor_num),
                action.sensor_dist,
                action.trn,
                true); // update position for route
        }

        if (action.delay != 0) {
            // ULOG("[train-notifier %s] delay %d ms\r\n",
            //     NOTIF_TYPE(type), action.delay * 10);
            Delay(clock_tid, action.delay);
        }

        // finished, let server perform action
        msg.type = type;
        Send(ptid, (char *)&msg, sizeof(train_msg), NULL, 0);
    }
}

static void
wait_action(int notif_tid, train_msg *msg, uint8_t trn, routing_action *action) {
    // assume at least one wait needed
    msg->type = MSG_TRAIN_ACTION;

    if (action->action_type == SENSOR) {
        msg->payload.route_action.sensor_dist = action->info.dist;
        msg->payload.route_action.sensor_num = action->sensor_num;
        msg->payload.route_action.delay = 0;
        msg->payload.route_action.trn = trn;
    } else {
        msg->payload.spd_action.sensor_dist = 0;
        msg->payload.spd_action.sensor_num = action->sensor_num;
        msg->payload.spd_action.delay = action->info.delay_ticks;
        msg->payload.spd_action.trn = trn;

        // for action execution
        memcpy(&msg->payload.spd_action.routing_action, action, sizeof(routing_action));
    }

    Send(notif_tid, (char *)msg, sizeof(train_msg), (char *)msg, sizeof(train_msg));
}

static void train_tc(void) {
    int ptid;
    train_msg msg;
    train_params params;

    int tc_server_tid = WhoIs(TC_SERVER_NAME);

    if (Receive(&ptid, (char *)&msg, sizeof(train_msg)) != sizeof(train_msg)) { // error, did not fully rcv?
        msg.type = MSG_TRAIN_ERROR;
        Reply(ptid, (char *)&msg, sizeof(train_msg));
        return;
    }

    memcpy(&params, &msg.payload.params, sizeof(train_params)); // copy over (before response!)
    msg.type = MSG_TRAIN_ACK;
    Reply(ptid, (char *)&msg, sizeof(train_msg));

    // register with coordinator
    if (track_control_register_train(tc_server_tid, MyTid(), params.trn,
        SENSOR_MOD(params.start->num), SENSOR_NO(params.start->num)) == -1) {
        // ULOG("[train] unable to register with coordinator");
        print_in_progress_message(WhoIs(CONSOLE_SERVER_NAME));
        return;
    }

    // create notifiers
    int route_notifier, spd_notifier;

    route_notifier = Create(P_MED, train_route_notifier);
    spd_notifier = Create(P_VHIGH, train_speed_notifier);

    // plan route
    routing_action_queue path, spd_changes;
    routing_action_queue_init(&path);
    routing_action_queue_init(&spd_changes);

    plan_route(params.start, params.end, params.offset, params.trn, SPD_STP, params.spd, &path, &spd_changes);

    // flags to not overwhelm notifiers and not to excessively block
    bool waiting_route = false, waiting_spd = false;

    int rcv_tid;
    routing_action routing_action;

    // throw any initial non-sensor dependent switches
    // before we start rolling
    while (!routing_action_queue_empty(&path)) {
        routing_action_queue_front(&path, &routing_action);
        if (routing_action.info.delay_ticks == 0 &&
            routing_action.sensor_num == SENSOR_NONE) {
            // pre-start switch?
            do_action(tc_server_tid, params.trn, &routing_action);
            routing_action_queue_pop_front(&path, NULL);
        } else {
            break; // no more
        }
    }

    for (;;) {
        // do actions until we can't anymore
        while (!waiting_spd && !routing_action_queue_empty(&spd_changes)) {
            routing_action_queue_pop_front(&spd_changes, &routing_action);
            if (routing_action.info.delay_ticks == 0 &&
                routing_action.sensor_num == SENSOR_NONE) {
                // no delay or notif, can just do it
                do_action(tc_server_tid, params.trn, &routing_action);
            } else {
                // will do action for us
                wait_action(spd_notifier, &msg, params.trn, &routing_action);
                waiting_spd = true;
            }
        }

        while (!waiting_route && !routing_action_queue_empty(&path)) {
            // guarantee that all wait on a sensor (implicit/explicit)
            routing_action_queue_front(&path, &routing_action);
            wait_action(route_notifier, &msg, params.trn, &routing_action);
            waiting_route = true;
        }

        // recalculate based on closest node that we are waiting on
        // if any

        // wait for a notifier
        uassert(Receive(&rcv_tid, (char *)&msg, sizeof(train_msg)) == sizeof(train_msg));
        Reply(rcv_tid, NULL, 0);

        switch (msg.type) {
            case MSG_TRAIN_NOTIFY_ROUTE: {
                waiting_route = false;
                // do all actions waiting on activated sensor
                routing_action_queue_front(&path, &routing_action);
                while (routing_action.sensor_num == msg.payload.route_action.sensor_num) {
                    // only pop off if actually equal
                    routing_action_queue_pop_front(&path, NULL);

                    do_action(tc_server_tid, params.trn, &routing_action);

                    // prime for next iteration
                    routing_action_queue_front(&path, &routing_action);
                }

                break;
            }
            case MSG_TRAIN_NOTIFY_SPD: {
                waiting_spd = false;
                // notifier executes action for us
                break;
            }
            default: {
                ULOG("[Train %d] Unknown notifier msg: %d\r\n", params.trn, msg.type);
                break;
            }
        }

        // check exit condition
        if (!waiting_route && !waiting_spd &&
            routing_action_queue_empty(&spd_changes) &&
            routing_action_queue_empty(&path)) {
            break;
        }
    }

    // shut down notifiers
    msg.type = MSG_TRAIN_QUIT;
    Send(route_notifier, (char *)&msg, sizeof(train_msg), NULL, 0);
    Send(spd_notifier, (char *)&msg, sizeof(train_msg), NULL, 0);

    // deregister from coordinator
    uassert(track_control_end_train(tc_server_tid, params.trn) == 0);
}

int CreateControlledTrain(uint8_t trn, enum SPEEDS spd,
    track_node *start, track_node *end, int32_t offset) {
    uassert(trn_hash(trn) != -1);
    uassert(start && end);
    uassert(track <= start || start < track + TRACK_MAX);
    uassert(track <= end || end < track + TRACK_MAX);

    uint16_t tc_tid = Create(P_MED, train_tc);

    train_msg msg;
    msg.type = MSG_TRAIN_PARAMS;
    msg.payload.params.trn = trn;
    msg.payload.params.spd = spd;
    msg.payload.params.start = start;
    msg.payload.params.end = end;
    msg.payload.params.offset = offset;

    int ret = Send(tc_tid, (char *)&msg, sizeof(train_msg), (char *)&msg, sizeof(train_msg));
    return (ret < 0 || msg.type == MSG_TRAIN_ERROR) ? -1 : tc_tid;
}
