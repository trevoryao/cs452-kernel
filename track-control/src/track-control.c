#include "track-control.h"

#include "msg.h"
#include "task.h"
#include "track-control-coordinator.h"

int track_control_register_train(int tid, int task_tid, uint16_t trainNo,
    uint16_t sensor_mod, uint16_t sensor_no) {
    uint16_t my_tid = MyTid();

    struct msg_tc_server msg;
    msg.type = MSG_TC_TRAIN_REGISTER;
    msg.requesterTid = my_tid;
    msg.data.trn_register.tid = task_tid;
    msg.data.trn_register.trn_no = trainNo;
    msg.data.trn_register.sensor.mod_sensor = sensor_mod;
    msg.data.trn_register.sensor.mod_num = sensor_no;

    int ret = Send(tid, (char *)&msg, sizeof(struct msg_tc_server), (char *)&msg, sizeof(struct msg_tc_server));

    if (ret < 0 || msg.type == MSG_TC_ERROR) {
        return -1;
    } else {
        return 0;
    }
}

int track_control_end_train(int tid, uint16_t trainNo) {
    uint16_t my_tid = MyTid();

    struct msg_tc_server msg;
    msg.type = MSG_TC_TRAIN_DONE;
    msg.requesterTid = my_tid;
    msg.data.trn_register.trn_no = trainNo;

    int ret = Send(tid, (char *)&msg, sizeof(struct msg_tc_server), NULL, 0);

    if (ret < 0 || msg.type == MSG_TC_ERROR) {
        return -1;
    } else {
        return 0;
    }
}

int16_t track_control_set_train_speed(int tid, uint16_t trainNo, uint16_t trainSpeed) {
    uint16_t my_tid = MyTid();

    struct msg_tc_server msg;
    msg.type = MSG_TC_TRAIN_PUT;
    msg.requesterTid = my_tid;
    msg.data.trn_cmd.trn_no = trainNo;
    msg.data.trn_cmd.spd = trainSpeed;

    int ret = Send(tid, (char *)&msg, sizeof(struct msg_tc_server), (char *)&msg, sizeof(struct msg_tc_server));

    if (ret < 0 || msg.type == MSG_TC_ERROR) {
        return -1;
    } else {
        return msg.data.trn_cmd.spd;
    }
}

int16_t track_control_get_train_speed(int tid, uint16_t trainNo) {
    uint16_t my_tid = MyTid();

    struct msg_tc_server msg;
    msg.type = MSG_TC_TRAIN_GET;
    msg.requesterTid = my_tid;
    msg.data.trn_cmd.trn_no = trainNo;

    int ret = Send(tid, (char *)&msg, sizeof(struct msg_tc_server), (char *)&msg, sizeof(struct msg_tc_server));

    if (ret < 0 || msg.type == MSG_TC_ERROR) {
        return -1;
    } else {
        return msg.data.trn_cmd.spd;
    }
}

void track_control_wait_sensor(int tid, uint16_t sensor_mod, uint16_t sensor_no) {
    uint16_t my_tid = MyTid();

    struct msg_tc_server msg;
    msg.type = MSG_TC_SENSOR_GET;
    msg.requesterTid = my_tid;
    msg.data.sensor.mod_sensor = sensor_mod;
    msg.data.sensor.mod_num = sensor_no;

    int ret = Send(tid, (char *)&msg, sizeof(struct msg_tc_server), (char *)&msg, sizeof(struct msg_tc_server));
}
void track_control_put_sensor(int tid, uint16_t sensor_mod, uint16_t sensor_no) {
    uint16_t my_tid = MyTid();

    struct msg_tc_server msg;
    msg.type = MSG_TC_SENSOR_PUT;
    msg.requesterTid = my_tid;
    msg.data.sensor.mod_sensor = sensor_mod;
    msg.data.sensor.mod_num = sensor_no;

    int ret = Send(tid, (char *)&msg, sizeof(struct msg_tc_server), (char *)&msg, sizeof(struct msg_tc_server));
}

int track_control_set_switch(int tid, uint16_t switch_no, enum SWITCH_DIR switch_dir) {
    uint16_t my_tid = MyTid();

    struct msg_tc_server msg;
    msg.type = MSG_TC_SWITCH_PUT;
    msg.requesterTid = my_tid;
    msg.data.sw_cmd.sw_no = switch_no;
    msg.data.sw_cmd.sw_dir = switch_dir;

    int ret = Send(tid, (char *)&msg, sizeof(struct msg_tc_server), (char *)&msg, sizeof(struct msg_tc_server));

    if (ret < 0 || msg.type == MSG_TC_ERROR) {
        return -1;
    } else {
        return msg.data.sw_cmd.sw_dir;
    }
}
