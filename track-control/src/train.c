#include "train.h"

#include "console-server.h"
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
#include "track-segment-locking.h"
#include "track-server.h"

extern track_node track[];

enum TRAIN_MSG_TYPE {
    MSG_TRAIN_PARAMS,
    MSG_TRAIN_ACTION,
    MSG_TRAIN_NOTIFY_ROUTE,
    MSG_TRAIN_NOTIFY_SPD,
    MSG_TRAIN_NOTIFY_LOCKING_TIMEOUT,
    MSG_TRAIN_NOTIFY_LOCKING_SUCCESS,
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

typedef struct locking_notif_action {
    decision_pt decision_pt;
    uint8_t trn;

    uint16_t segmentIDs[MAX_SEGMENTS_MSG];
    uint8_t no_segments;
} locking_notif_action;

typedef struct train_msg {
    enum TRAIN_MSG_TYPE type;

    union {
        train_params params;
        // a request is either a sensor or a timestamp or both (but never neither)
        route_notif_action route_action;
        spd_notif_action spd_action;
        locking_notif_action locking_action;
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
    int console_tid = WhoIs(CONSOLE_SERVER_NAME);

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
            int ret = track_control_wait_sensor(tc_server_tid,
                SENSOR_MOD(action.sensor_num),
                SENSOR_NO(action.sensor_num),
                action.sensor_dist,
                action.trn, false); // don't update position for speed
            if (ret < 0) {
                print_missed_sensor(console_tid, action.trn,
                    SENSOR_MOD(action.sensor_num) - 1,
                    SENSOR_NO(action.sensor_num) - 1);
            } else {
                clear_missed_sensor(console_tid, action.trn);
            }
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
    int console_tid = WhoIs(CONSOLE_SERVER_NAME);

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
            int ret = track_control_wait_sensor(tc_server_tid,
                SENSOR_MOD(action.sensor_num),
                SENSOR_NO(action.sensor_num),
                action.sensor_dist,
                action.trn,
                true); // update position for route
            if (ret < 0) {
                print_missed_sensor(console_tid, action.trn,
                    SENSOR_MOD(action.sensor_num) - 1,
                    SENSOR_NO(action.sensor_num) - 1);
            } else {
                clear_missed_sensor(console_tid, action.trn);
            }
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
    uassert(msg->type == MSG_TRAIN_ACK);
}

static void train_locking_notifier(void) {
    int ptid;
    train_msg msg;

    int tc_server_tid = WhoIs(TC_SERVER_NAME);
    int console_tid = WhoIs(CONSOLE_SERVER_NAME);
    int locking_server_tid = WhoIs(TS_SERVER_NAME);

    locking_notif_action action;

    for (;;) {
        uassert(Receive(&ptid, (char *)&msg, sizeof(train_msg)) == sizeof(train_msg));

        enum TRAIN_MSG_TYPE rcvd_type = msg.type;

        switch (rcvd_type) {
            case MSG_TRAIN_ACTION:
                memcpy(&action, &msg.payload.locking_action, sizeof(locking_notif_action));
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
        if (action.decision_pt.sensor_num != SENSOR_NONE) {
            // ULOG("[train-notifier-%s] wait on sensor %c%d\r\n",
            //     NOTIF_TYPE(type),
            //     SENSOR_MOD(action.decision_pt.sensor_num) - 1 + 'A',
            //     SENSOR_NO(action.decision_pt.sensor_num));
            // Delay(clock_tid, 10);
            int ret = track_control_wait_sensor(tc_server_tid,
                SENSOR_MOD(action.decision_pt.sensor_num),
                SENSOR_NO(action.decision_pt.sensor_num),
                0,
                action.trn, false); // don't update position for speed
            if (ret < 0) {
                print_missed_sensor(console_tid, action.trn,
                    SENSOR_MOD(action.decision_pt.sensor_num) - 1,
                    SENSOR_NO(action.decision_pt.sensor_num) - 1);
            } else {
                clear_missed_sensor(console_tid, action.trn);
            }
        }

        deque segments;
        deque_init(&segments, 3);

        // acquire lock with given timeout
        for (uint8_t i = 0; i < action.no_segments; ++i) {
            deque_push_back(&segments, action.segmentIDs[i]);
        }

        // return result back to server
        bool res = track_server_lock_all_segments_timeout(locking_server_tid,
            &segments, action.trn, action.decision_pt.ticks);
        msg.type = res ? MSG_TRAIN_NOTIFY_LOCKING_SUCCESS : MSG_TRAIN_NOTIFY_LOCKING_TIMEOUT;

        Send(ptid, (char *)&msg, sizeof(train_msg), NULL, 0);
    }
}

inline static void
wait_decision_pt(int notif_tid, train_msg *msg, route *route, int trn) {
    msg->type = MSG_TRAIN_ACTION;
    memcpy(&msg->payload.locking_action.decision_pt, &route->decision_pt, sizeof(decision_pt));
    msg->payload.locking_action.trn = trn;

    msg->payload.locking_action.no_segments = 0;
    for (deque_itr it = deque_begin(&route->segments);
        it != deque_end(&route->segments);
        it = deque_itr_next(it)) {
        msg->payload.locking_action.segmentIDs
            [msg->payload.locking_action.no_segments++] =
            deque_itr_get(&route->segments, it);
    }

    Send(notif_tid, (char *)msg, sizeof(train_msg), (char *)msg, sizeof(train_msg));
    uassert(msg->type == MSG_TRAIN_ACK);
}

// returns the new value of stopped -- i.e. true if train has had to stop
// false otherwise
// updates next_segment to point to the start of the next desired segment
// even in emergency stop, to keep up to date on our current position
// (the planner will handle any possible reversing for us)
static bool execute_plan(route *cur_route, route *prev_route, int tc_server_tid,
    int locking_server_tid, train_params *params,
    __attribute__((unused)) track_node *next_segment,
    int route_notifier, int spd_notifier, int locking_notifier) {
    // flags to not overwhelm notifiers and not to excessively block
    bool waiting_route = false, waiting_spd = false;

    int rcv_tid;
    routing_action routing_action;
    train_msg msg;

    // flag to free previous segments after first entering new segments
    bool segments_freed = false;

    // throw any initial non-sensor dependent switches
    // before we start rolling
    while (!routing_action_queue_empty(&cur_route->path)) {
        routing_action_queue_front(&cur_route->path, &routing_action);
        if (routing_action.info.delay_ticks == 0 &&
            routing_action.sensor_num == SENSOR_NONE) {
            // pre-start switch?
            do_action(tc_server_tid, params->trn, &routing_action);
            routing_action_queue_pop_front(&cur_route->path, NULL);
        } else {
            break; // no more
        }
    }

    // start-up locking notifier (separate from any other route actions)
    if (cur_route->decision_pt.sensor_num != SENSOR_NONE)
        wait_decision_pt(locking_notifier, &msg, cur_route, params->trn);

    // set next_segment (because of when we timeout, can guarantee)
    routing_action_queue_back(&cur_route->path, &routing_action);
    next_segment = &track[routing_action.sensor_num];

    for (;;) {
        // do actions until we can't anymore
        while (!waiting_spd && !routing_action_queue_empty(&cur_route->speed_changes)) {
            routing_action_queue_pop_front(&cur_route->speed_changes, &routing_action);
            if (routing_action.info.delay_ticks == 0 &&
                routing_action.sensor_num == SENSOR_NONE) {
                // no delay or notif, can just do it
                do_action(tc_server_tid, params->trn, &routing_action);
            } else {
                // will do action for us
                wait_action(spd_notifier, &msg, params->trn, &routing_action);
                waiting_spd = true;
            }
        }

        while (!waiting_route && !routing_action_queue_empty(&cur_route->path)) {
            // guarantee that all wait on a sensor (implicit/explicit)
            routing_action_queue_front(&cur_route->path, &routing_action);
            wait_action(route_notifier, &msg, params->trn, &routing_action);
            waiting_route = true;
        }

        // wait for a notifier
        uassert(Receive(&rcv_tid, (char *)&msg, sizeof(train_msg)) == sizeof(train_msg));
        Reply(rcv_tid, NULL, 0);

        switch (msg.type) {
            case MSG_TRAIN_NOTIFY_LOCKING_SUCCESS: {
                break; // Nothing to do -- could theoretically plan next route already, but not really needed
            }
            case MSG_TRAIN_NOTIFY_LOCKING_TIMEOUT: {
                // emergency stop
                track_control_set_train_speed(tc_server_tid, params->trn, SP_REVERSE);
                track_control_set_train_speed(tc_server_tid, params->trn, SP_REVERSE);

                return true;
            }
            case MSG_TRAIN_NOTIFY_ROUTE: {
                waiting_route = false;

                // do all actions waiting on activated sensor
                routing_action_queue_front(&cur_route->path, &routing_action);
                while (routing_action.sensor_num == msg.payload.route_action.sensor_num) {
                    // only pop off if actually equal
                    routing_action_queue_pop_front(&cur_route->path, NULL);

                    do_action(tc_server_tid, params->trn, &routing_action);

                    // prime for next iteration
                    routing_action_queue_front(&cur_route->path, &routing_action);
                }

                // free segments at end, make sure we have indeed completely left
                if (!segments_freed) {
                    track_server_free_segments(locking_server_tid, &prev_route->segments, params->trn);
                    segments_freed = true; // only once
                }

                break;
            }
            case MSG_TRAIN_NOTIFY_SPD: {
                waiting_spd = false;
                // notifier executes action for us
                break;
            }
            default: {
                ULOG("[Train %d] Unknown notifier msg: %d\r\n", params->trn, msg.type);
                break;
            }
        }

        // check exit condition
        if (!waiting_route && !waiting_spd &&
            routing_action_queue_empty(&cur_route->speed_changes) &&
            routing_action_queue_empty(&cur_route->path)) {
            break;
        }
    }

    // if we have executed the final segment, we have stopped (albeit regularly)
    return cur_route->state != NORMAL_SEGMENT;
}

static void train_tc(void) {
    int ptid;
    train_msg msg;
    train_params params;

    int tc_server_tid = WhoIs(TC_SERVER_NAME);
    int console_server_tid = WhoIs(CONSOLE_SERVER_NAME);
    int locking_server_tid = WhoIs(TS_SERVER_NAME);

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
        print_in_progress_message(console_server_tid);
        return;
    }

    // create notifiers
    int route_notifier, spd_notifier, lock_notifier;

    route_notifier = Create(P_MED, train_route_notifier);
    spd_notifier = Create(P_VHIGH, train_speed_notifier);
    lock_notifier = Create(P_VHIGH, train_locking_notifier);

    // flags for tracking train attributes
    bool stopped = false;
    bool reversed = false;

    // lock initial segment
    // guarantee that we are the only train in our segment (i hope)
    track_server_lock_segment(locking_server_tid, params.start->reverse->segmentId, params.trn);

    // use two routes at all times
    // either use cur as current in use route and prev := 1 - cur
    // OR: use both from stopped
    route routes[2];
    routing_actions_init(&routes[0]);
    routing_actions_init(&routes[1]);

    int8_t cur = 0; // current index in routes array

    // in "previous" itr, locked only starting segment
    deque_push_back(&routes[cur].segments, params.start->reverse->segmentId);

    deque prev_segments; // used after stopping

    track_node *current_node = params.start;

    for (;;) {
        // plan next segment
        if (stopped) {
            // plan double segment and wait until either free
            // save segments we need to unlock after determining a route
            deque_move(&prev_segments, &routes[cur].segments);

            // plan both routes
            plan_stopped_route(current_node, params.end, params.offset, params.trn,
                params.spd, locking_server_tid, &routes[0], &routes[1]);


            // wait on either forward or reverse
            // returns index of whichever one was acquired
            cur = track_server_lock_two_all_segments(locking_server_tid,
                &routes[0].segments, &routes[1].segments, params.trn);
            uassert(cur != -1);
            reversed = (cur == 1); // save for done

            if (reversed) {
                track_control_set_train_speed(tc_server_tid, params.trn, SP_REVERSE);
            }
        } else {
            // save old info for unlocking
            cur = 1 - cur;

            // already have lock from previous iteration
            plan_in_progress_route(current_node, params.end, params.offset, params.trn,
                params.spd, locking_server_tid, &routes[cur]);
        }

        // execute itr
        stopped = execute_plan(&routes[cur], &routes[1 - cur], tc_server_tid,
            locking_server_tid, &params, current_node, route_notifier,
            spd_notifier, lock_notifier);

        if (stopped && reversed) {
            // return to regular state only if we had to stop
            track_control_set_train_speed(tc_server_tid, params.trn, SP_REVERSE);
            reversed = false;
        }

        if (routes[cur].state == FINAL_SEGMENT) break;
    }

    // shut down notifiers
    msg.type = MSG_TRAIN_QUIT;
    Send(route_notifier, (char *)&msg, sizeof(train_msg), NULL, 0);
    Send(spd_notifier, (char *)&msg, sizeof(train_msg), NULL, 0);
    Send(lock_notifier, (char *)&msg, sizeof(train_msg), NULL, 0);

    // deregister from coordinator
    uassert(track_control_end_train(tc_server_tid, params.trn) == 0);
}

int CreateControlledTrain(uint8_t trn, track_node *start,
    track_node *end, int32_t offset) {
    uassert(trn_hash(trn) != -1);
    uassert(start && end);
    uassert(track <= start || start < track + TRACK_MAX);
    uassert(track <= end || end < track + TRACK_MAX);

    uint16_t tc_tid = Create(P_MED, train_tc);

    train_msg msg;
    msg.type = MSG_TRAIN_PARAMS;
    msg.payload.params.trn = trn;
    msg.payload.params.spd = SPD_MED; // default spd, changed by TCC
    msg.payload.params.start = start;
    msg.payload.params.end = end;
    msg.payload.params.offset = offset;

    int ret = Send(tc_tid, (char *)&msg, sizeof(train_msg), (char *)&msg, sizeof(train_msg));
    return (ret < 0 || msg.type == MSG_TRAIN_ERROR) ? -1 : tc_tid;
}
