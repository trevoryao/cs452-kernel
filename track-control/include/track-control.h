#ifndef __TRACK_CONTROL_H__
#define __TRACK_CONTROL_H__

#include <stdbool.h>
#include <stdint.h>

#include "controller-consts.h"

// returns -1 if already registered (or any other error)
int track_control_register_train(int tid, int task_tid, uint16_t trainNo, uint16_t sensor_mod, uint16_t sensor_no);
int track_control_end_train(int tid, uint16_t trainNo);

int16_t track_control_set_train_speed(int tid, uint16_t trainNo, uint16_t trainSpeed);
int16_t track_control_get_train_speed(int tid, uint16_t trainNo);

void track_control_wait_sensor(int tid, uint16_t sensor_mod, uint16_t sensor_no);
void track_control_put_sensor(int tid, uint16_t sensor_mod, uint16_t sensor_no);

int track_control_set_switch(int tid, uint16_t switch_no, enum SWITCH_DIR switch_dir);


#endif
