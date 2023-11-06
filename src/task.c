#include "task.h"

// lib
#include "syscall.h"

// wrappers on syscall ASM f'n

int Create(int priority, void (*function)()) {
    return syscall(SYS_CREAT, priority, (int64_t)function, 0, 0, 0, 0, 0);
}

int MyTid(void) {
    return syscall(SYS_TID, 0, 0, 0, 0, 0, 0, 0);
}

int MyParentTid(void) {
    return syscall(SYS_PTID, 0, 0, 0, 0, 0, 0, 0);
}

void Yield(void) {
    syscall(SYS_YIELD, 0, 0, 0, 0, 0, 0, 0);
}

void Exit(void) {
    syscall(SYS_EXIT, 0, 0, 0, 0, 0, 0, 0);
}

void KillAllChildren(void) {
    syscall(SYS_KILL_CHILD, 0, 0, 0, 0, 0, 0, 0);
}

void GetIdleStatus(uint64_t *idle_sys_ticks, uint64_t *total_sys_ticks) {
    syscall(SYS_IDLE_STATUS, (int64_t)idle_sys_ticks, (int64_t)total_sys_ticks, 0, 0, 0, 0, 0);
}
