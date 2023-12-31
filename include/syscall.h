#ifndef __SYSCALL_H__
#define __SYSCALL_H__

#include <stdint.h>

// codes for syscalls
enum SYSCALL_N {
    SYS_CREAT = 1,
    SYS_TID,
    SYS_PTID,
    SYS_YIELD,
    SYS_EXIT,
    SYS_IDLE_STATUS,
    SYS_KILL,
    SYS_KILL_CHILD,
    SYS_MSG_SEND,
    SYS_MSG_RCV,
    SYS_MSG_RPLY,
    SYS_AWAIT
};

// stub f'n for syscalls
extern int syscall(enum SYSCALL_N n, int64_t arg1, int64_t arg2, int64_t arg3, int64_t arg4,
    int64_t arg5, int64_t arg6, int64_t arg7);

#endif
