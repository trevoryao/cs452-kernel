#include "task-state.h"

// kernel
#include "context-switch.h"
#include "event-queue.h"
#include "interrupts.h"
#include "kassert.h"
#include "task-alloc.h"
#include "task-queue.h"
#include "stack-alloc.h"
#include "stopwatch.h"

// lib
#include "task.h"
#include "syscall.h"
#include "util.h"

// dummy routine to handle end of user function
void user_start(void (*function)(void)) {
    KLOG("task-%x start\r\n", function);
    function();
    Exit();
}

void task_init(task_t *t, void (*function)(), task_t *parent, enum PRIORITY priority, stack_alloc *salloc) {
    t->x0 = (uint64_t)function; // pass param to user_start
    t->x1 = 0;
    t->x2 = 0;
    t->x3 = 0;
    t->x4 = 0;
    t->x5 = 0;
    t->x6 = 0;
    t->x7 = 0;
    t->x8 = 0;
    t->x9 = 0;
    t->x10 = 0;
    t->x11 = 0;
    t->x12 = 0;
    t->x13 = 0;
    t->x14 = 0;
    t->x15 = 0;
    t->x16 = 0;
    t->x17 = 0;
    t->x18 = 0;
    t->x19 = 0;
    t->x20 = 0;
    t->x21 = 0;
    t->x22 = 0;
    t->x23 = 0;
    t->x24 = 0;
    t->x25 = 0;
    t->x26 = 0;
    t->x27 = 0;
    t->x28 = 0;
    t->x29 = 0;
    t->x30 = 0;

    t->pc = (uint64_t)user_start;
    t->sp = (uint64_t)stack_alloc_new(salloc);

    t->pstate = 0;

    // tid assigned at scheduler
    t->parent = parent;

    t->priority = priority;
    t->ready_state = STATE_READY;

    t->stack_base = (void *)t->sp;
}

uint8_t task_activate(task_t *t, kernel_state *k) {
    return context_switch_out(t, k);
}

void task_svc_handle(task_t *t, task_alloc *talloc, stack_alloc *salloc,
    task_queue *tq, event_queue *eq, stopwatch *stopwatch) {
    KLOG("task-%d SYSCALL(%d) %x %x %x %x %x\r\n", t->tid, t->x0, t->x1, t->x2, t->x3, t->x4, t->x5);

    switch (t->x0) {
        case SYS_CREAT:
            if (0 <= t->x1 && t->x1 < N_PRIORITY) { // valid priority?
                task_t *new_task;
                if (!(new_task = task_alloc_new(talloc))) {
                    t->x0 = -2; // out of mem
                    break;
                }

                task_init(new_task, (void (*)(void))t->x2, t, t->x1, salloc);

                int32_t tid;
                if ((tid = task_queue_add(tq, new_task)) < 0) {
                    t->x0 = -2; // out of tids
                    break;
                }

                t->x0 = tid; // return new tid
            } else {
                t->x0 = -1; // invalid priority
            }

            break;
        case SYS_TID:
            t->x0 = t->tid;
            break;
        case SYS_PTID:
            if (t->parent) { // has parent
                t->x0 = t->parent->tid;
            } else {
                // killed parent or kernel parent
                t->x0 = 0;
            }
            break;
        case SYS_YIELD:
            break; // let scheduler handle, already moved to back of queue
        case SYS_EXIT:
            t->ready_state = STATE_KILLED;
            break;
        case SYS_KILL_CHILD:
            task_queue_kill_children(tq, t->tid);
            break;
        case SYS_IDLE_STATUS:
            *(uint64_t *)t->x1 = stopwatch_get_total_ticks(stopwatch, STPW_IDLE_TASK);
            *(uint64_t *)t->x2 = stopwatch_get_total_ticks(stopwatch, STPW_USER_TASK);
            break;
        case SYS_MSG_SEND: {
            task_t *rcv_t;
            if (!(rcv_t = task_queue_get(tq, t->x1))) { // no such receiver task?
                t->x0 = -1; // return no such TID
                break;
            }

            if (rcv_t->ready_state == STATE_RCV_WAIT) { // receive called first?
                t->ready_state = STATE_RPLY_WAIT; // block waiting for receiver task to reply

                // copy data from sender (t) to receiver

                // calculate number of chars to copy (take min of Send msglen
                // & receive msglen)
                size_t len = (t->x3 < rcv_t->x3) ? t->x3 : rcv_t->x3;
                memcpy((void *)rcv_t->x2, (void *)t->x2, len);

                *(int *)rcv_t->x1 = t->tid; // set tid of sending task to receiver
                rcv_t->x0 = len; // return msg size stored in dest

                rcv_t->ready_state = STATE_READY; // unblock receiver task
            } else { // send called first
                t->ready_state = STATE_SEND_WAIT; // block, wait for receive

                // add to back of receiver task's queue
                task_t *cur = rcv_t->waiting_senders_next,
                    *prev = NULL;
                while (cur) {
                    prev = cur;
                    cur = rcv_t->waiting_senders_next;
                }

                if (!prev) { // empty queue?
                    rcv_t->waiting_senders_next = t;
                } else {
                    prev->waiting_senders_next = t;
                }
            }

            break;
        }
        case SYS_MSG_RCV: {
            if (t->waiting_senders_next) { // waiting senders => send called first?
                // pop first off the queue
                task_t *send_t = t->waiting_senders_next;
                t->waiting_senders_next = send_t->waiting_senders_next;
                send_t->waiting_senders_next = NULL;

                send_t->ready_state = STATE_RPLY_WAIT;

                // calculate number of chars to copy (take min of Send msglen
                // & receive msglen)
                size_t len = (t->x3 < send_t->x3) ? t->x3 : send_t->x3;
                memcpy((void *)t->x2, (void *)send_t->x2, len);

                *(int *)t->x1 = send_t->tid; // set tid of sending task to receiver

                t->x0 = len; // return msg size stored in dest
            } else { // rcv called first
                t->ready_state = STATE_RCV_WAIT; // block until send is called
            }

            break;
        }
        case SYS_MSG_RPLY: {
            task_t *send_t;
            if (!(send_t = task_queue_get(tq, t->x1))) { // no such sender task?
                t->x0 = -1; // return no such TID
                break;
            } else if (send_t->ready_state != STATE_RPLY_WAIT) { // task not reply-blocked?
                t->x0 = -2;
                break;
            }

            // calculate number of chars to copy (take min of Send rplen
            // & reply rplen)
            size_t len = (send_t->x5 < t->x3) ? send_t->x5 : t->x3;
            memcpy((void *)send_t->x4, (void *)t->x2, len);

            t->x0 = len; // return msg size copied into sender mem

            send_t->x0 = len; // return msg size copied into sender mem
            send_t->ready_state = STATE_READY; // unblock sender

            break;
        }
        case SYS_AWAIT: {
            if (event_queue_block(eq, t, (int)t->x1)) { // valid event?
                trigger_interrupt((enum EVENT)t->x1);
                t->x0 = 0; // return may be changed later
            } else {
                t->x0 = -1; // return error
            }

            break;
        }
        default: kpanic("Unknown syscall number: %d\r\n", t->x0);
    }
}

void task_clear(task_t *t) {
    t->next = NULL;
    t->waiting_senders_next = NULL;
    t->event_wait_next = NULL;
    t->tid = 0;
}

void task_dealloc(task_t *t, task_alloc *talloc, stack_alloc *salloc, task_queue *tq) {
    task_queue_free_tid(tq, t->tid);
    stack_alloc_free(salloc, t->stack_base);
    task_alloc_free(talloc, t);
}

void kernel_state_init(kernel_state *k) {
    memset(k, 0, sizeof(kernel_state));
}
