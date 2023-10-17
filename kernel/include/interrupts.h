#ifndef __INTERRUPTS_H__
#define __INTERRUPTS_H__

// lib
#include "events.h"

typedef struct event_queue event_queue;

/*
 * Methods used to handle interrupts
 */

// must be called in init
void init_interrupt_handlers(void);

void trigger_interrupt(enum EVENT e);

void handle_interrupt(event_queue *eq);

#endif
