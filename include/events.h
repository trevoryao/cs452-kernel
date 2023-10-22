#ifndef __EVENTS_H__
#define __EVENTS_H__

// helpful enum for supported events
enum EVENT {
    TIMER_TICK,
    MARKLIN_RX,
    MARKLIN_CTS,
    N_EVENTS
};

/*
 * blocks until the event identified by eventid occurs then returns
 * with event-specific data, if any.
 *
 * Return Value
 *  - >=0	event-specific data, in the form of a positive integer.
 *  - -1	invalid event.
 */
int AwaitEvent(int eventid);

#endif
