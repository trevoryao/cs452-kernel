#include "program-tasks.h"

#include <stdint.h>

#include "clock.h"
#include "deque.h"
#include "nameserver.h"
#include "msg.h"
#include "task.h"
#include "time.h"
#include "uart-server.h"
#include "uassert.h"

#include "control-msgs.h"
#include "monitor.h"
#include "speed.h"
#include "track.h"
#include "track-data.h"

extern track_node track[];

void time_task_main(void) {
    uint16_t clock_tid = WhoIs(CLOCK_SERVER_NAME);
    uint16_t console_tid = WhoIs(CONSOLE_SERVER_NAME);

    uint32_t real_time = Time(clock_tid);
    uint32_t target = real_time;

    time_t time;

    for (;;) {
        time_from_clock_ticks(&time, real_time);
        update_time(console_tid, &time);

        target += REFRESH_TIME;
        real_time = DelayUntil(clock_tid, target);
    }
}

void idle_time_task_main(void) {
    uint16_t clock_tid = WhoIs(CLOCK_SERVER_NAME);
    uint16_t console_tid = WhoIs(CONSOLE_SERVER_NAME);

    uint32_t real_time = Time(clock_tid);
    uint32_t target = real_time;

    uint64_t idle_sys_ticks, user_sys_ticks;

    for (;;) {
        GetIdleStatus(&idle_sys_ticks, &user_sys_ticks);
        update_idle(console_tid, idle_sys_ticks, user_sys_ticks);

        target += IDLE_REFRESH_TIME;
        real_time = DelayUntil(clock_tid, target);
    }
}
