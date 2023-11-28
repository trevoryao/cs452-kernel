#include "snake.h"

#include "clock.h"
#include "deque.h"
#include "msg.h"
#include "nameserver.h"
#include "uart-server.h"
#include "uassert.h"

#include "controller-consts.h"
#include "sensor-queue.h"
#include "sensor-worker.h"
#include "speed.h"
#include "track-data.h"
#include "track.h"
#include "user.h"

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

#define SNAKE_LEN 2

void snake_server_main(void) {
    uassert(RegisterAs(SNAKE_NAME) == 0);

    int clock = WhoIs(CLOCK_SERVER_NAME);
    int console = WhoIs(CONSOLE_SERVER_NAME);
    int marklin = WhoIs(MARKLIN_SERVER_NAME);
    int user_server = WhoIs(USER_SERVER_NAME);

    uint8_t snake_arr[N_TRNS] = {0};
    uint8_t snake_head = 0;

    speed_t spd_t;
    speed_t_init(&spd_t);

    sensor_queue sensor_queue;
    sensor_queue_init(&sensor_queue, &spd_t);

    int rtid;
    snake_msg msg;

    track_node *next = NULL;

    // for now just get two from runner
    snake_head = SNAKE_LEN - 1; // must do backwards for now
    for (int i = 0; i < SNAKE_LEN; ++i) {
        uassert(Receive(&rtid, (char *)&msg, sizeof(snake_msg)) ==
            sizeof(snake_msg));

        snake_arr[snake_head - i] = msg.trn;
        train_mod_speed(marklin, &spd_t, msg.trn, SPD_VLO);

        if (i == 0) { // for now just assume single start node
            next = msg.sensor;
            Printf(console, "First Sensor: %s\r\n", next->name);
            for (uint8_t i = snake_head; i > 0; --i) {
                sensor_queue_wait(&sensor_queue, next, snake_arr[i]);
            }
        }

        if (i != snake_head) {
            Delay(clock, 75);
        }

        Reply(rtid, NULL, 0);
    }

    // start up a sensorWorker
    Create(P_SENSOR_WORKER, sensor_worker_main);

    for (;;) {
        // get data from sensor worker
        int trn_gaps = 0;
        while (trn_gaps < snake_head) {
            uassert(Receive(&rtid, (char *)&msg, sizeof(snake_msg)) ==
                sizeof(snake_msg));

            switch (msg.type) {
                case SNAKE_MSG_SENSOR: {
                    uassert(Reply(rtid, NULL, 0) == 0);

                    int64_t time_between = sensor_queue_update(&sensor_queue,
                        msg.sensor, msg.time);

                    if (time_between == FIRST_ACTIVATION) {
                        next = user_reached_sensor(user_server, next);
                        uassert(next);
                        for (uint8_t i = snake_head; i > 0; --i) {
                            sensor_queue_wait(&sensor_queue, next, snake_arr[i]);
                        }
                    } else if (time_between > 0) {
                        Printf(console, "time between trains %d <- %d @ %s: %dms\r\n",
                            snake_arr[snake_head - trn_gaps],
                            snake_arr[snake_head - trn_gaps - 1],
                            msg.sensor->name, time_between * 10);
                        ++trn_gaps;
                    }

                    break;
                }
                default: upanic("unknown msg type %d from sensor worker", msg.type);
            }
        }
    }
}
