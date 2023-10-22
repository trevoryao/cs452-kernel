#ifndef __TASK_H__
#define __TASK_H__

/*
 * Kernel Task Creation
 * All functions cause rescheduling
 */

enum PRIORITY {
    P_IDLE,
    P_LOW,
    P_MED,
    P_HIGH,
    P_VHIGH,
    P_SERVER_LO,
    P_SERVER_HI,
    P_NOTIF,
    N_PRIORITY
};

 /*
 * allocates and initializes a task descriptor, using the given priority, and the
 * given function pointer as a pointer to the entry point of executable code,
 * essentially a function with no arguments and no return value. When Create returns,
 * the task descriptor has all the state needed to run the task, the task’s stack
 * has been suitably initialized, and the task has been entered into its ready
 * queue so that it will run the next time it is scheduled.
 *
 * Return Value
 *    tid	the positive integer task id of the newly created task. The task id
 *          must be unique, in the sense that no other active task has the same task id.
 *    -1	invalid priority.
 *    -2	kernel is out of task descriptors.
 *
 * Note: If newly created tasks need to receive configuration parameters,
 *       these generally need to be provided via message passing. However,
 *       if the configuration value is a primitive type, implementing a shortcut
 *       via another argument to Create() and function() is permissible
 */

int Create(int priority, void (*function)());

/*
 * returns the task id of the calling task.
 *
 * Return Value
 *    tid	the positive integer task id of the task that calls it.
 *
 * Note: No errors possible
 */
int MyTid(void);

/*
 * returns the task id of the task that created the calling task. This will
 * be problematic only if the parent task has exited or been destroyed,
 * in which case the return value is implementation-dependent.
 *
 * Return Value
 *    tid	the task id of the task that created the calling task.
 *
 * Note: return value is implementation-dependent, if the parent has exited,
 *       has been de- stroyed, or is in the process of being destroyed.
 */
int MyParentTid(void);

/*
 * causes a task to pause executing. The task is moved to the end of
 * its priority queue, and will resume executing when next scheduled.
 */
void Yield(void);

/*
 * causes a task to cease execution permanently. It is removed from
 * all priority queues, send queues, receive queues and event queues.
 * Resources owned by the task, primarily its memory and task descriptor,
 * may be reclaimed.
 */
void Exit(void);

/*
 * causes all children task to cease execution immediately (i.e.
 * exit is called for all child tasks)
 */
void KillAllChildren(void);

#endif
