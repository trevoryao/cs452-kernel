#ifndef __SENSORS_H__
#define __SENSORS_H__

#include <stdint.h>

typedef struct deque deque;

/*
 * interface for sending bytes to the controller
 * all f'ns take tid of marklin output server and
 * add/remove necessary bytes to the output deque
 */

// struct for counting rcv'd sensor data
typedef struct sensor {
    uint16_t mod_num; // which sensor to dump
    uint16_t mod_sensor; // which sensor byte to process
} sensor;

void sen_data_init(sensor *d);

// start sensor dump
void sen_start_dump(uint16_t tid);

// processes a byte of received data
// returns 1 if no more bytes left to receive for dump
// if so, call sen_start_dump
int rcv_sen_dump(sensor *d, char b, uint16_t tid, deque *recent_sens);

#endif
