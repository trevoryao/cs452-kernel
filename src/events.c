#include "events.h"

// lib
#include "syscall.h"

int AwaitEvent(int eventid) {
    // don't bother verifying here, since only wrapper on syscall (speed)
    return syscall(SYS_AWAIT, eventid, 0, 0, 0, 0, 0, 0);
}
