#ifndef __SNAKE_H__
#define __SNAKE_H__

#include <stdint.h>

typedef struct track_node track_node;

#define SNAKE_NAME "snake"

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
