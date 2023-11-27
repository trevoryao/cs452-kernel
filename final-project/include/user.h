#ifndef __USER_H__
#define __USER_H__

#include <stdint.h>

struct track_node;

void user_server_main(void);

void user_display_distance(int32_t um);
track_node *user_get_next_sensor(void);
uint8_t user_reached_sensor(track_node *track); // return train to start up, if any

#endif
