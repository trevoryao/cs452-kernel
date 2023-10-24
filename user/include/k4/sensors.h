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
typedef struct sen_data {
    uint16_t mod_num; // which sensor to dump
    uint16_t mod_round; // which sensor byte to process
} sen_data;

void sen_data_init(sen_data *d);

// start sensor dump
void sen_start_dump(uint16_t tid);

// processes a byte of received data
// returns 1 if no more bytes left to receive for dump
// if so, call sen_start_dump
int rcv_sen_dump(sen_data *d, char b, uint16_t tid, deque *recent_sens);

#endif
