#include "k4/monitor.h"

#include "clock-server.h"
#include "console-server.h"
#include "marklin-server.h"
#include "nameserver.h"
#include "msg.h"
#include "task.h"
#include "time.h"
#include "uart-server.h"
#include "uassert.h"

#include "k4/control-msgs.h"
#include "k4/program-tasks.h"
#include "k4/track.h"

void manual_track_controller(void) {
    // start up clock, uart servers
    Create(P_SERVER_HI, clockserver_main);

    uint16_t console_tid = Create(P_SERVER_LO, console_server_main);
    uint16_t marklin_tid = Create(P_SERVER_HI, marklin_server_main);

    // initialisation commands
    init_monitor(console_tid);
    init_track(marklin_tid);

    // start all tasks
    Create(P_HIGH, time_task_main);
    Create(P_MED, cmd_task_main);
    Create(P_HIGH, sensor_task_main);

    // wait for quit control msg from cmd_task
    int cmd_task_tid;
    msg_control msg;
    uassert(Receive(&cmd_task_tid, (char *)&msg, sizeof(msg_control)) == sizeof(msg_control));
    msg.type = MSG_CONTROL_ACK;
    uassert(Reply(cmd_task_tid, (char *)&msg, sizeof(msg_control)) == sizeof(msg_control));

    // shutdown all non-server children to exit gracefully
    KillAllChildren();

    // send shutdown commands
    shutdown_monitor(console_tid);
    shutdown_track(marklin_tid);
}
