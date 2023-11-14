#ifndef __TRACK_SEGMENT_LOCKING_H__
#define __TRACK_SEGMENT_LOCKING_H__

#include <stdbool.h>
#include <stdint.h>
#include "track-node.h"

bool track_server_available(int tid);

int track_server_acquire_server_lock(int tid);

int track_server_free_server_lock(int tid);

int track_server_lock_segment(int tid, track_node *node, int direction);

int track_server_free_segment(int tid, track_node *node);


#endif
