#ifndef __TRAIN_H__
#define __TRAIN_H__

#include <stdint.h>

#include "speed.h"

typedef struct track_node track_node;

/*
 * Interface for controlling a single train from origin to destination
 * Implemented as a worker with blocking notifiers
 * Awaiting on sensor activations and timer interrupts respectively.
 *
 * Returns -1 on error, and the created TID otherwise
 *
 * N.B: Returns without guarantee of completion
 */

int CreateControlledTrain(uint8_t trn, track_node *start,
    track_node *end, int32_t offset);

#endif
