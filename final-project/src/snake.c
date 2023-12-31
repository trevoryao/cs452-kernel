#include "snake.h"

#include "clock.h"
#include "deque.h"
#include "msg.h"
#include "nameserver.h"
#include "uart-server.h"
#include "uassert.h"
#include "util.h"

#include "monitor.h"
#include "sensor-queue.h"
#include "sensor-worker.h"
#include "speed-data.h"
#include "speed.h"
#include "track-data.h"
#include "track.h"
#include "user.h"

extern speed_data spd_data;
extern track_node track[];

enum SNAKE_MSG_TYPE {
    SNAKE_MSG_START,
    SNAKE_MSG_END,
    SNAKE_MSG_SENSOR,
    SNAKE_MSG_TIMER_START,
    SNAKE_MSG_TIMER_END,
    N_SNAKE_MSG
};

typedef struct snake_msg {
    enum SNAKE_MSG_TYPE type;

    // num for external, snake_idx for internal
    union {
        uint8_t num;
        uint8_t snake_idx;
    } trn;

    track_node *sensor;
    int64_t time;
} snake_msg;

void snake_server_start(int tid, uint8_t trn, track_node *start) {
    snake_msg msg = {SNAKE_MSG_START, {.num = trn}, start, 0};
    uassert(Send(tid, (char *)&msg, sizeof(snake_msg), NULL, 0) == 0);
}

void snake_server_end(int tid) {
    snake_msg msg = {SNAKE_MSG_END, {0}, NULL, 0};
    uassert(Send(tid, (char *)&msg, sizeof(snake_msg), NULL, 0) == 0);
}

void snake_server_sensor_triggered(int tid, track_node *sensor,
    uint32_t time) {
    snake_msg msg = {SNAKE_MSG_SENSOR, {0}, sensor, time};
    uassert(Send(tid, (char *)&msg, sizeof(snake_msg), NULL, 0) == 0);
}

static void snake_timer_notifier(void);

void snake_init(snake *snake) {
    for (int i = 0; i < N_TRNS; ++i) {
        snake->trns[i].trn = 0;
        snake->trns[i].queued_spd_adjustment = 0;
        snake->trns[i].reverse = (int8_t)false;
        snake->trns[i].grace_period = TIME_NONE;
        snake->trns[i].last_dist_between = DIST_NONE;
        snake->trns[i].curr_dist_between = DIST_NONE;
    }

    snake->head = 0;
    snake->waiting_timer = (int8_t)false;

    speed_t_init(&snake->spd_t);

    snake->clock = WhoIs(CLOCK_SERVER_NAME);
    snake->console = WhoIs(CONSOLE_SERVER_NAME);
    snake->marklin = WhoIs(MARKLIN_SERVER_NAME);
    snake->user = WhoIs(USER_SERVER_NAME);

    // start up notifier for grace period timing
    snake->timer = Create(P_HIGH, snake_timer_notifier);
}

static int32_t
calculate_distance_between(snake *snake, uint8_t front_trn_idx,
    uint32_t time_between) {

    snake_trn_data *data = &snake->trns[front_trn_idx];
    // assume constant speed over small distances
    // first calculate distance between front of each sensor module
    int32_t distance_between_heads =
        get_distance_from_velocity(&spd_data, data->trn, time_between,
            speed_display_get(&snake->spd_t, data->trn));

    if (data->reverse) {
        distance_between_heads = (distance_between_heads < (REVERSE_OFFSET * MM_TO_UM)) ?
            0 : (distance_between_heads - (REVERSE_OFFSET * MM_TO_UM));
    }

    // must adjust to measure distance between trains lengths
    return (distance_between_heads < (TRN_LEN_MM * MM_TO_UM)) ?
        0 : (distance_between_heads - (TRN_LEN_MM * MM_TO_UM));
}

static void
snake_try_wait_timer(snake *snake) {
    if (snake->waiting_timer) return; // only wait if not already waiting
    // find earliest time we need to wait on
    int64_t min_time = INT64_MAX;
    int8_t min_idx = -1;

    uint8_t trn_idx = 0;
    while (trn_idx <= snake->head) {
        if (snake->trns[trn_idx].grace_period != TIME_NONE &&
            snake->trns[trn_idx].grace_period < min_time) {
            min_time = snake->trns[trn_idx].grace_period;
            min_idx = trn_idx;
        }

        ++trn_idx;
    }

    if (min_idx == -1) return; // nothing to wait for

    snake->waiting_timer = true; // prevent double wait
    snake_msg msg = {SNAKE_MSG_TIMER_START, {.snake_idx = min_idx}, NULL, min_time};
    uassert(Send(snake->timer, (char *)&msg, sizeof(snake_msg), NULL, 0) == 0);
}

static void
snake_quit_timer_notifier(snake *snake) {
    snake_msg msg = {SNAKE_MSG_END, {0}, NULL, 0};
    uassert(Send(snake->timer, (char *)&msg, sizeof(snake_msg), NULL, 0) == 0);
}

#define ADJUST_SPD_UP 1
#define ADJUST_NONE 0
#define ADJUST_SLOW_DOWN -1

static void
snake_try_make_queued_speed_adjustment(snake *snake, uint8_t activated_snake_idx) {
    uint8_t activated_trn = snake->trns[activated_snake_idx].trn;

    if (snake->trns[activated_snake_idx].queued_spd_adjustment != ADJUST_NONE &&
        snake->trns[activated_snake_idx].grace_period == TIME_NONE) {
        // assume already checked to be valid
        uint32_t time = Time(snake->clock);
        int8_t spd_initial = speed_display_get(&snake->spd_t, activated_trn);
        int8_t spd_desired = spd_initial + snake->trns[activated_snake_idx].queued_spd_adjustment;
        train_mod_speed(snake->marklin, &snake->spd_t, activated_trn,spd_desired);
        update_speed(snake->console, &snake->spd_t, activated_trn);
        update_snake_train(snake->console, snake, activated_snake_idx);

        // populate grace_period (2 * train length) -> time
        uint32_t grace_period = estimate_initial_time_acceleration(&spd_data,
            activated_trn, spd_initial, spd_desired, (TRN_LEN_MM << 1) * MM_TO_UM);
        if (grace_period > MAX_GRACE_PERIOD) {
            grace_period = MAX_GRACE_PERIOD;
        }

        snake->trns[activated_snake_idx].grace_period = time + grace_period; // absolute time
        // clear adjustment only after

        snake->trns[activated_snake_idx].queued_spd_adjustment = ADJUST_NONE; // clear now

        // wait if necessary
        snake_try_wait_timer(snake);

        if (activated_snake_idx == snake->head)
            user_updated_head_speed(snake->user, activated_trn, spd_desired);
    }
}

// adds adjustments to the speeds of trains in the snake in-front of current position
// only if speed is supported
static void
snake_change_speed_fwd(snake *snake, uint8_t front_trn_idx, int8_t adjustment_factor) {
    // must save for trains in front of us (including front_idx)
    for (; front_trn_idx <= snake->head; ++front_trn_idx) {
        if (!speed_is_supported(speed_display_get(&snake->spd_t,
            snake->trns[front_trn_idx].trn) + adjustment_factor)) {
            continue;
        }

        if (adjustment_factor > ADJUST_NONE &&
            snake->trns[front_trn_idx].curr_dist_between >= (FOLLOWING_DIST_MM +
            FOLLOWING_DIST_MARGIN_MM) * MM_TO_UM) {
            // don't carry through acceleration if already too far apart
            continue;
        }

        snake->trns[front_trn_idx].queued_spd_adjustment = adjustment_factor;
    }
}

// checks if all trains ahead can support the possible action (without changing)
static bool
snake_check_change_speed_fwd(snake *snake, uint8_t front_trn_idx, int8_t adjustment_factor) {
    while (front_trn_idx <= snake->head) {
        if (!speed_is_supported(speed_display_get(&snake->spd_t,
            snake->trns[front_trn_idx++].trn) + adjustment_factor)) {
            return false;
        }
    }

    return true;
}

// adds adjustments to the speeds of trains in the snake behind of current position
// only if speed is supported
static void
snake_change_speed_behind(snake *snake, uint8_t front_trn_idx, int8_t adjustment_factor) {
    // must save for trains one past us, change back of the gap (one behind) now

    // start with back of the gap
    for (int8_t trn_idx = front_trn_idx - 1; trn_idx >= 0; --trn_idx) {
        if (!speed_is_supported(speed_display_get(&snake->spd_t,
            snake->trns[trn_idx].trn) + adjustment_factor)) {
            continue;
        }

        if (adjustment_factor > ADJUST_NONE &&
            snake->trns[trn_idx].curr_dist_between >= (FOLLOWING_DIST_MM +
            FOLLOWING_DIST_MARGIN_MM) * MM_TO_UM) {
            // don't carry through acceleration if already too far apart
            continue;
        }

        snake->trns[trn_idx].queued_spd_adjustment = adjustment_factor;
    }

    // can do back of the gap immediately
    snake_try_make_queued_speed_adjustment(snake, front_trn_idx - 1);
}

// checks if all trains behind can support the possible action (without changing)
static bool
snake_check_change_speed_behind(snake *snake, uint8_t front_trn_idx, int8_t adjustment_factor) {
    int8_t trn_idx = front_trn_idx - 1; // start with back of the gap

    while (trn_idx >= 0) {
        if (!speed_is_supported(speed_display_get(&snake->spd_t,
            snake->trns[trn_idx--].trn) + adjustment_factor)) {
            return false;
        }
    }

    return true;
}

static bool
snake_check_matching_trend(snake *snake, uint8_t front_trn_idx,
    int8_t adjustment_factor) {

    uint8_t trn = snake->trns[front_trn_idx].trn;
    int32_t trend_margin = get_velocity(&spd_data, trn, speed_display_get(&snake->spd_t, trn)) >> 1;

    int32_t diff = (snake->trns[front_trn_idx].curr_dist_between -
        snake->trns[front_trn_idx].last_dist_between) * MM_TO_UM;

    if (-trend_margin <= diff &&
        diff <= trend_margin) {
        // small enough range, do nothing, so say we are matching the trend
        return true;
    } else if (diff > trend_margin) {
        // upward trend
        return adjustment_factor > ADJUST_NONE; // matching increase trend
    } else {
        // downward trend
        return adjustment_factor < ADJUST_NONE;
    }
}

// true if all bunched and adjusted all the trains, false otherwise
static bool
snake_try_adjust_bunched_trains(snake *snake) {
    // assume at least 3 trains
    if (snake->head < 2) return false;

    // are we all bunched?
    for (int i = 1; i <= snake->head; ++i) {
        if (snake->trns[i].curr_dist_between > (SMALL_FOLLOWING_DIST * MM_TO_UM))
            return false;
    }

    // all bunched up
    // will this work with more than three trains? hard to say
    snake_change_speed_fwd(snake, (snake->head >> 1) + 1, ADJUST_SPD_UP);
    snake_change_speed_behind(snake, (snake->head >> 1) + 1, ADJUST_SLOW_DOWN);

    return true;
}

// called as the subsequent train passes the triggered sensor
static void
snake_adjust_trains(snake *snake, uint8_t front_trn_idx) {
    // first check the trends
    int32_t dist_between = snake->trns[front_trn_idx].curr_dist_between;
    int32_t trend = snake->trns[front_trn_idx].curr_dist_between -
        snake->trns[front_trn_idx].last_dist_between;

    if (snake_try_adjust_bunched_trains(snake)) {
        return;
    }

    if (trend <= -LARGE_TREND * MM_TO_UM) {
        // closing into each other, same as under small following distance
        snake_change_speed_fwd(snake, front_trn_idx, ADJUST_SPD_UP);
        snake_change_speed_behind(snake, front_trn_idx, ADJUST_SLOW_DOWN);
    } else if (dist_between >= LARGE_FOLLOWING_DIST * MM_TO_UM) {
        // need to drastically decrease where we can
        if (snake_check_matching_trend(snake, front_trn_idx, ADJUST_SLOW_DOWN))
            return;

        snake_change_speed_fwd(snake, front_trn_idx, ADJUST_SLOW_DOWN);
        snake_change_speed_behind(snake, front_trn_idx, ADJUST_SPD_UP);
    } else {
        int8_t fwd_adjustment, behind_adjustment;

        if (dist_between > (FOLLOWING_DIST_MM + FOLLOWING_DIST_MARGIN_MM) * MM_TO_UM) {
            // need to decrease
            if (snake_check_matching_trend(snake, front_trn_idx, ADJUST_SLOW_DOWN))
                return;

            fwd_adjustment = ADJUST_SLOW_DOWN;
            behind_adjustment = ADJUST_SPD_UP;
        } else if (dist_between < (FOLLOWING_DIST_MM - FOLLOWING_DIST_MARGIN_MM) * MM_TO_UM) {
            // must increase
            if (dist_between > SMALL_FOLLOWING_DIST * MM_TO_UM && snake_check_matching_trend(snake, front_trn_idx, ADJUST_SPD_UP))
                return;

            fwd_adjustment = ADJUST_SPD_UP;
            behind_adjustment = ADJUST_SLOW_DOWN;
        } else {
            // within our margin -- do nothing
            return;
        }

        // favour forward actions
        if (snake_check_change_speed_behind(snake, front_trn_idx, behind_adjustment)) {
            snake_change_speed_behind(snake, front_trn_idx, behind_adjustment);
        } else if (snake_check_change_speed_fwd(snake, front_trn_idx, fwd_adjustment)) {
            snake_change_speed_fwd(snake, front_trn_idx, fwd_adjustment);
        } else {
            // can't do all -- default to just doing behind, since we are at the front, maybe have some behind
            snake_change_speed_behind(snake, front_trn_idx, behind_adjustment);
            // emergency stop and restart front train?
        }
    }
}

static void
snake_wait_on_sensor_queue(snake *snake, track_node *next, sensor_queue *sq) {
    if (snake->head == 0) {
        // single train
        sensor_queue_wait(sq, next, snake->head, true);
    } else {
        for (uint8_t i = snake->head; i > 0; --i) {
            sensor_queue_wait(sq, next, i, false);
        }
    }
}

void snake_timer_notifier(void) {
    uint16_t clock = WhoIs(CLOCK_SERVER_NAME);
    uint16_t ptid = MyParentTid();

    snake_msg msg;
    int tid;

    for (;;) {
        uassert(Receive(&tid, (char *)&msg, sizeof(snake_msg)) == sizeof(snake_msg));
        if (tid != ptid) {
            upanic("[snake-notifier] unexpected msg from tid %d (parent tid %d)", tid, ptid);
        }

        Reply(ptid, NULL, 0);

        switch (msg.type) {
            case SNAKE_MSG_END: return;
            case SNAKE_MSG_TIMER_START: {
                msg.type = SNAKE_MSG_TIMER_END; // do before waiting
                DelayUntil(clock, msg.time);
                uassert(Send(ptid, (char *)&msg,
                    sizeof(snake_msg), NULL, 0) == 0);
                break;
            }
            default: upanic("[snake-notifier] unknown msg type %d", msg.type);
        }
    }
}

void snake_server_main(void) {
    uassert(RegisterAs(SNAKE_NAME) == 0);

    snake snake;
    snake_init(&snake);

    sensor_queue sensor_queue;
    sensor_queue_init(&sensor_queue, &snake);

    int rtid;
    snake_msg msg;

    track_node *next = NULL;

    // wait for user to give us first train
    uassert(Receive(&rtid, (char *)&msg, sizeof(snake_msg)) ==
        sizeof(snake_msg));
    Reply(rtid, NULL, 0);

    // save metadata
    snake.trns[snake.head].trn = msg.trn.num;
    next = msg.sensor;
    user_updated_head_speed(snake.user, snake.trns[snake.head].trn, SPD_MED);

    // start up a sensorWorker
    int sensor_worker_tid = Create(P_SENSOR_WORKER, sensor_worker_main);

    // start train
    train_mod_speed(snake.marklin, &snake.spd_t, msg.trn.num, SPD_MED);
    update_speed(snake.console, &snake.spd_t, msg.trn.num);
    init_snake_train(snake.console, &snake, snake.head);

    // wait on start node
    snake_wait_on_sensor_queue(&snake, next, &sensor_queue);

    for (;;) {
        // get data from sensor worker
        uassert(Receive(&rtid, (char *)&msg, sizeof(snake_msg)) ==
            sizeof(snake_msg));
        uassert(Reply(rtid, NULL, 0) == 0);

        switch (msg.type) {
            case SNAKE_MSG_SENSOR: {
                uint8_t activated_snake_idx;
                int64_t time_between = sensor_queue_update(&sensor_queue,
                    msg.sensor, msg.time, &activated_snake_idx);

                if (time_between == FIRST_ACTIVATION) {
                    // perform any queued speed change
                    snake_try_make_queued_speed_adjustment(&snake, activated_snake_idx);

                    // prime next
                    uint8_t next_trn;
                    int32_t next_dist;
                    uint8_t reverse;
                    next = user_reached_sensor(snake.user, next,
                        &next_trn, &next_dist, &reverse);
                    uassert(next);

                    if (next_trn != 0) {
                        // new pick up
                        snake.trns[++snake.head].trn = next_trn;
                        snake.trns[snake.head].reverse = (int8_t)reverse;

                        // start train
                        train_mod_speed(snake.marklin, &snake.spd_t, next_trn, SPD_MED);

                        if (reverse) {
                            // user reverses on our behalf already (need *a* delay)
                            train_mod_speed(snake.marklin, &snake.spd_t, next_trn, SP_REVERSE);
                            train_mod_speed(snake.marklin, &snake.spd_t, next_trn, SPD_MED);
                        }

                        update_speed(snake.console, &snake.spd_t, next_trn);
                        init_snake_train(snake.console, &snake, snake.head);
                    }

                    snake_wait_on_sensor_queue(&snake, next, &sensor_queue);
                } else if (time_between > 0) {
                    int32_t dist_between = calculate_distance_between(&snake, activated_snake_idx, time_between);

                    // buffer
                    snake.trns[activated_snake_idx].last_dist_between =
                        snake.trns[activated_snake_idx].curr_dist_between;
                    snake.trns[activated_snake_idx].curr_dist_between =
                        dist_between;

                    update_snake_distance(snake.console, &snake, msg.sensor->num, activated_snake_idx, dist_between / MM_TO_UM);

                    // plan/do any adjustments (now and in the future)
                    snake_adjust_trains(&snake, activated_snake_idx);
                }
                break;
            }
            case SNAKE_MSG_TIMER_END: {
                // clear ending timer
                snake.waiting_timer = false;
                snake.trns[msg.trn.snake_idx].grace_period = TIME_NONE;

                // start new one if needed
                snake_try_wait_timer(&snake);
                break;
            }
            case SNAKE_MSG_END: {
                snake_quit_timer_notifier(&snake);
                KillChild(sensor_worker_tid);
                return; // EXIT
            }
            default: upanic("unknown snake msg type %d", msg.type);
        }
    }
}
