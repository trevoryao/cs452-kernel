#include "msg.h"

#include "syscall.h"

// wrappers on syscall f'n

int Send(int tid, const char *msg, int msglen, char *reply, int rplen) {
    return syscall(SYS_MSG_SEND, tid, (int64_t)msg, msglen, (int64_t)reply, rplen, 0, 0);
}

int Receive(int *tid, char *msg, int msglen) {
    return syscall(SYS_MSG_RCV, (int64_t)tid, (int64_t)msg, msglen, 0, 0, 0, 0);
}

int Reply(int tid, const char *reply, int rplen) {
    return syscall(SYS_MSG_RPLY, tid, (int64_t)reply, rplen, 0, 0, 0, 0);
}
