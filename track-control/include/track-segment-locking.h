#ifndef __TRACK_SEGMENT_LOCKING_H__
#define __TRACK_SEGMENT_LOCKING_H__

#include <stdbool.h>
#include <stdint.h>
#include "track-node.h"
#include "deque.h"

typedef struct deque deque;

#define TS_SERVER_NAME "track-server"


/*
 * Max deque length = 8 (msg sent as fixed-length array)
 * Locks all of the given segment IDs, or none
 * Blocks until timeout (if specified) or
 * until all segment locks acquired
 */
bool track_server_lock_all_segments_timeout(int tid, deque *segmentIDs, uint16_t trainNo, uint32_t timeout_ticks);
void track_server_lock_all_segments(int tid, deque *segmentIDs, uint16_t trainNo);

/*
 * Max deque length = 8 (msg sent as fixed-length array)
 * Locks ONE of the given segment IDs, or none
 * Blocks until timeout (if specified) or
 * until a segment lock acquired
 *
 * Returns the segmentID of the acquired segment 
 *      (returns -1 if failed)
 */
int track_server_lock_one_segment_timeout(int tid, deque *segmentIDs, uint16_t trainNo, uint32_t timeout_ticks);
int track_server_lock_one_segment(int tid, deque *segmentIDs, uint16_t trainNo);

/*
 * Max deque length = 8 (msg sent as fixed-length array)
 * Frees lock on all segments specified
 */
void track_server_free_segments(int tid, deque *segmentIDs, uint16_t trainNo);

// returns true if the given segmentID is locked, false otherwise
bool track_server_segment_is_locked(int tid, int segmentID, uint16_t trainNo);

#endif
