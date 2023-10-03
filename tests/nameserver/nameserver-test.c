#include "nameserver.h"
#include "util.h"
#include "msg.h"
#include "msg-type.h"
#include "rpi.h"
#include "task.h"
#include "uassert.h"

#define N_TEST_TASKS 9

void test_task(void) {
    int my_tid = MyTid();

    char name[32] = "Task-";
    i2a(my_tid, name + 6);

    int ret = RegisterAs(name);
    uassert(ret == 0);

    ret = WhoIs(name);
    uassert(ret == my_tid);
}

void user_main(void) {
    for (int i = 0; i < N_TEST_TASKS; ++i) {
        Create(P_MED, test_task);
    }
}
