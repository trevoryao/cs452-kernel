#include "deque.h"
#include "rpi.h"
#include "uassert.h"
#include "util.h"

#include "controller-consts.h"
#include "speed-data.h"
#include "track-data.h"
#include "position.h"
#include "clock.h"
#include "nameserver.h"
#include "task.h"
#include "clock-server.h"

speed_data spd_data;
track_node track[TRACK_MAX];


void user_main(void) {
    int clockTid = Create(P_SERVER_HI, clockserver_main);
    speed_data_init(&spd_data);
    init_track_b(track);

    int trainNo = 77;
    int distance = 350000;
    trn_position trn_pos;

    uart_printf(CONSOLE, "Starting Position Testing\r\n");

    trn_position_init(&trn_pos);

    uint32_t time = Time(clockTid);
    trn_position_update_train_speed(&trn_pos, trainNo, 7, time);
    
    
    trn_position_reached_next_sensor(&trn_pos, trainNo, time+10);

    trn_position_update_next_expected_pos(&trn_pos, trainNo, distance);
    trn_position_reached_next_sensor(&trn_pos, trainNo, time+310, 1, 1);
    trn_position_update_train_speed(&trn_pos, trainNo, 9, time+310);

    trn_position_update_next_expected_pos(&trn_pos, trainNo, distance+100);
    

    trn_position_reached_next_sensor(&trn_pos, trainNo, time+1000, 1, 1);
    //trn_position_update_train_speed(&trn_pos, trainNo, 7, time+600);
    //trn_position_update_train_speed(&trn_pos, trainNo, 9, time+600);







}