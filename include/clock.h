#ifndef __CLOCK_H__
#define __CLOCK_H__

#include <stdint.h>
#include "msg-type.h"

enum CLOCK_MSG_TYPE {
    MSG_CLOCKSERVER_TIME,
    MSG_CLOCKSERVER_DELAY,
    MSG_CLOCKSERVER_DELAY_UNTIL,
    MSG_CLOCKSERVER_NOTIFY,
    MSG_CLOCKSERVER_AWAKE,
    MSG_CLOCKSERVER_OK,
    MSG_CLOCKSERVER_ERROR,
    MSG_CLOCKSERVER_MAX
};

typedef struct msg_clockserver {
    enum CLOCK_MSG_TYPE type;
    uint16_t requesterTid;
    uint64_t clockTick;
} msg_gameserver;

/*
* returns the number of ticks since the clock server was created and initialized.
* With a 10 millisecond tick and a 32-bit unsigned int for the time wraparound
* is almost 12,000 hours, plenty of time for your demo.
* Time is actually a wrapper for a send to the clock server.
* The argument is the tid of the clock server.
*
* Return Value:
* >=0	time in ticks since the clock server initialized.
* -1	tid is not a valid clock server task.
*/
int Time(int tid);

/*
* Blocks the caller until at least until the specified number of ticks has elapsed
* (since the Delay call), according to the time server identified by tid.
* The actual wait time is not guaranteed because the caller may have to wait on
* higher priority tasks.
*
* Return Value:
* >=0	success. The current time returned (as in Time())
* -1	tid is not reachable or is not a time server
* -2	negative delay.
*/
int Delay(int tid, int ticks);


/*
* Blocks the caller at least until the specified number of ticks has elapsed since
* the initialization of time server tid. How long after is not guaranteed because
* the caller may have to wait on higher priority tasks.
* DelayUntil(tid, Time(tid) + ticks)) is similar to Delay(tid, ticks)).
*
* Return Value:
* >=0	success. The current time returned (as in Time())
* -1	tid is not reachable or is not a time server
* -2	negative delay.
*/
int DelayUntil(int tid, int ticks);

#endif
