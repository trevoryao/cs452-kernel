#ifndef __MONITOR_H__
#define __MONITOR_H__

#include "controller-consts.h"

#include <stdint.h>

typedef struct deque deque;
typedef struct speed_t speed_t;
typedef struct time_t time_t;

/*
 * Program interface is as follows:
 |---------------------------------------------------------|
 | Time                                 Recent Sensors:    |
 |                                           sensor1       |
 | Switches:                                 sensor2       |
 |  SW1: X    SW2: X   SW3: X   SW4: X       sensor3       |
 |  SW5: X    SW6: X   SW7: X   SW8: X       sensor4       |
 |  SW9: X   SW10: X  SW11: X  SW12: X       sensor5       |
 | SW13: X   SW14: X  SW15: X  SW16: X       sensor6       |
 | SW17: X   SW18: X                         sensor7       |
 | SW0x99: X          SW0x9A: X              sensor8       |
 | SW0x9B: X          SW0x9C: X              sensor9       |
 |                                                         |
 | cmd>                                                    |
 |---------------------------------------------------------|
 */

// f'ns take the tid of the console I/O server

// must be called prior to any update f'ns
void init_monitor(uint16_t tid);
void shutdown_monitor(uint16_t tid);

// pretty self-explanatory
void print_prompt(uint16_t tid);
void update_time(uint16_t tid, time_t *t);
void update_idle(uint16_t tid, uint64_t idle_sys_ticks, uint64_t user_sys_ticks);
void update_switch(uint16_t tid, uint16_t sw, enum SWITCH_DIR dir);
void update_speed(uint16_t tid, speed_t *spd_t, uint16_t tr);

// caller responsible for retaining state q of recently triggered sensors
void update_triggered_sensor(uint16_t tid, deque *q, uint16_t sen_mod, uint16_t sen_no);

void cmd_delete(uint16_t tid); // delete prev char before cursor
void cmd_out(uint16_t tid, char c); // echo back cmd

void print_error_message(uint16_t tid);
void reset_error_message(uint16_t tid);

void print_in_progress_message(uint16_t tid);

void print_train_time(uint16_t tid, int8_t trainNo, int32_t diff, int8_t sensor_mod, int8_t sensor_no);
void print_train_location_error(uint16_t tid, int8_t trainNo, int8_t exp_sensor_mod, int8_t exp_sensor_no, int8_t actual_sensor_mod, int8_t actual_sensor_no);


#endif
