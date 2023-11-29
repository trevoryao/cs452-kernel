#ifndef __SNAKE_H__
#define __SNAKE_H__

#include <stdint.h>

#include "controller-consts.h"
#include "speed.h"

typedef struct track_node track_node;

#define SNAKE_NAME "snake"

static const int FOLLOWING_DIST_MM = 100;
static const int FOLLOWING_DIST_MARGIN_MM = 25;
static const int TREND_MARGIN_UM = 15000; // 15mm
static const int LARGE_TREND = 100; // mm
static const int LARGE_FOLLOWING_DIST = 3 * FOLLOWING_DIST_MM;
static const int SMALL_FOLLOWING_DIST = 10;

typedef struct snake_trn_data {
    uint8_t trn;
    int8_t queued_spd_adjustment;
    int64_t grace_period;

    int32_t last_dist_between;
    int32_t curr_dist_between;
} snake_trn_data;

// structure for holding metadata about the current state of the snake
typedef struct snake {
    // trains in the snake
    snake_trn_data trns[N_TRNS];
    uint8_t head;

    // speed structure (global singleton)
    speed_t spd_t;

    int8_t waiting_timer; // flag if we are currently waiting on a timer

    // TIDs (saves param passing)
    uint16_t clock, console, marklin, user, timer;
} snake;

void snake_init(snake *snake);

/*
 * Wrapper f'ns for passing messages to the server
 */

// required on start-up
// trn is the first head of the snake, and start is the first sensor
// it passes
void snake_server_start(int tid, uint8_t trn, track_node *start);

// called by sensor worker whenever new sensor activation is triggered
void snake_server_sensor_triggered(int tid, track_node *sensor, uint32_t time);

void snake_server_main(void);

#endif
