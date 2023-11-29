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
    SNAKE_MSG_SENSOR,
    N_SNAKE_MSG
};

typedef struct snake_msg {
    enum SNAKE_MSG_TYPE type;
    uint8_t trn;
    track_node *sensor;
    uint32_t time;
} snake_msg;

void snake_server_start(int tid, uint8_t trn, track_node *start) {
    snake_msg msg = {SNAKE_MSG_START, trn, start, 0};
    uassert(Send(tid, (char *)&msg, sizeof(snake_msg), NULL, 0) == 0);
}

void snake_server_sensor_triggered(int tid, track_node *sensor,
    uint32_t time) {
    snake_msg msg = {SNAKE_MSG_SENSOR, 0, sensor, time};
    uassert(Send(tid, (char *)&msg, sizeof(snake_msg), NULL, 0) == 0);
}

void snake_init(snake *snake) {
    memset(snake->trns, 0, sizeof(snake_trn_data) * N_TRNS);
    snake->head = 0;

    speed_t_init(&snake->spd_t);

    snake->clock = WhoIs(CLOCK_SERVER_NAME);
    snake->console = WhoIs(CONSOLE_SERVER_NAME);
    snake->marklin = WhoIs(MARKLIN_SERVER_NAME);
    snake->user = WhoIs(USER_SERVER_NAME);
}

static int32_t
calculate_distance_between(speed_t *spd_t, uint8_t front_trn,
    uint32_t time_between) {
    // assume constant speed over small distances
    // first calculate distance between front of each sensor module
    int32_t distance_between_heads =
        get_distance_from_velocity(&spd_data, front_trn, time_between,
            speed_display_get(spd_t, front_trn));

    // must adjust to measure distance between trains lengths
    return (distance_between_heads < (TRN_LEN_MM * MM_TO_UM)) ?
        0 : (distance_between_heads - (TRN_LEN_MM * MM_TO_UM));
}

// adds adjustments to the speeds of trains in the snake in-front of current position
// only if speed is supported
static void
snake_change_speed_fwd(snake *snake, uint8_t front_trn_idx, int8_t adjustment_factor) {
    // must save for trains in front of us (including front_idx)
    while (front_trn_idx <= snake->head) {
        if (speed_is_supported(speed_display_get(&snake->spd_t,
            snake->trns[front_trn_idx].trn) + adjustment_factor)) {
            snake->trns[front_trn_idx++].queued_spd_adjustment = adjustment_factor;
        }
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
    int8_t trn_idx = front_trn_idx - 1; // start with back of the gap

    // can do back of the gap immediately
    if (speed_is_supported(speed_display_get(&snake->spd_t,
        snake->trns[trn_idx].trn) + adjustment_factor)) {
        train_mod_speed(snake->marklin, &snake->spd_t, snake->trns[trn_idx].trn,
            speed_display_get(&snake->spd_t, snake->trns[trn_idx].trn) + adjustment_factor);
        update_speed(snake->console, &snake->spd_t, snake->trns[trn_idx--].trn);
    }

    while (trn_idx >= 0) {
        if (speed_is_supported(speed_display_get(&snake->spd_t,
            snake->trns[trn_idx].trn) + adjustment_factor)) {
            snake->trns[trn_idx--].queued_spd_adjustment = adjustment_factor;
        }
    }
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

#define ADJUST_SPD_UP 1
#define ADJUST_SLOW_DOWN -1

// called as the subsequent train passes the triggered sensor
static void
snake_adjust_trains(snake *snake, uint8_t front_trn_idx, int32_t dist_between) {
    if (dist_between >= LARGE_FOLLOWING_DIST * MM_TO_UM) {
        // need to drastically decrease where we can
        snake_change_speed_fwd(snake, front_trn_idx, ADJUST_SLOW_DOWN);
        snake_change_speed_behind(snake, front_trn_idx, ADJUST_SPD_UP);
    } else {
        int8_t fwd_adjustment, behind_adjustment;

        if (dist_between > (FOLLOWING_DIST_MM + FOLLOWING_DIST_MARGIN_MM) * MM_TO_UM) {
            // need to decrease
            fwd_adjustment = ADJUST_SLOW_DOWN;
            behind_adjustment = ADJUST_SPD_UP;
        } else if (dist_between < (FOLLOWING_DIST_MM - FOLLOWING_DIST_MARGIN_MM) * MM_TO_UM) {
            // must increase
            fwd_adjustment = ADJUST_SPD_UP;
            behind_adjustment = ADJUST_SLOW_DOWN;
        } else {
            // within our margin -- do nothing
            return;
        }

        // favour behind actions
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
    ULOG("waiting on %s\r\n", next->name);
    if (snake->head == 0) {
        // single train
        sensor_queue_wait(sq, next, snake->head, true);
    } else {
        for (uint8_t i = snake->head; i > 0; --i) {
            sensor_queue_wait(sq, next, i, false);
        }
    }
}

#define SNAKE_LEN 1 // testing only

void snake_server_main(void) {
    uassert(RegisterAs(SNAKE_NAME) == 0);

    snake snake;
    snake_init(&snake);

    sensor_queue sensor_queue;
    sensor_queue_init(&sensor_queue, &snake);

    int rtid;
    snake_msg msg;

    track_node *next = NULL;

    // for now just get two from runner
    snake.head = SNAKE_LEN - 1; // must do backwards for now
    for (int i = 0; i < SNAKE_LEN; ++i) {
        uassert(Receive(&rtid, (char *)&msg, sizeof(snake_msg)) ==
            sizeof(snake_msg));

        snake.trns[snake.head - i].trn = msg.trn;
        train_mod_speed(snake.marklin, &snake.spd_t, msg.trn, SPD_MED);
        update_speed(snake.console, &snake.spd_t, msg.trn);

        if (i == 0) { // for now just assume single start node
            next = msg.sensor;
        }

        if (i != snake.head) {
            Delay(snake.clock, 75);
        }

        Reply(rtid, NULL, 0);
    }

    // wait on start node
    snake_wait_on_sensor_queue(&snake, next, &sensor_queue);

    // start up a sensorWorker
    Create(P_SENSOR_WORKER, sensor_worker_main);

    uint8_t activated_snake_idx;

    for (;;) {
        // get data from sensor worker
        uassert(Receive(&rtid, (char *)&msg, sizeof(snake_msg)) ==
            sizeof(snake_msg));
        uassert(msg.type == SNAKE_MSG_SENSOR);
        uassert(Reply(rtid, NULL, 0) == 0);

        int64_t time_between = sensor_queue_update(&sensor_queue,
            msg.sensor, msg.time, &activated_snake_idx);

        if (time_between == FIRST_ACTIVATION) {
            // prime next
            next = user_reached_sensor(snake.user, next);
            uassert(next);
            snake_wait_on_sensor_queue(&snake, next, &sensor_queue);

            // perform any queued speed change
            // how to perform queued speed change with only trn and next trn? need to keep idx's
        } else if (time_between > 0) {
            int32_t dist_between = calculate_distance_between(&snake.spd_t, snake.trns[activated_snake_idx].trn, time_between);
            Printf(snake.console, "distance between trains %d <- %d @ %s: %dmm\r\n",
                snake.trns[activated_snake_idx],
                snake.trns[activated_snake_idx - 1],
                msg.sensor->name, dist_between / MM_TO_UM);
        }
    }
}
