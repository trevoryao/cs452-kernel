#ifndef __TRACK_NODE_H__
#define __TRACK_NODE_H__

typedef enum {
  NODE_NONE,
  NODE_SENSOR,
  NODE_BRANCH,
  NODE_MERGE,
  NODE_ENTER,
  NODE_EXIT,
} node_type;

enum direction_lock {
  DIR_A = 0, // Clock wise
  DIR_B, // Counter clock
  DIR_NONE, 
  DIR_EMPTY
};

#define DIR_AHEAD 0
#define DIR_STRAIGHT 0
#define DIR_CURVED 1

struct track_node;
typedef struct track_node track_node;
typedef struct track_edge track_edge;

struct track_edge {
  track_edge *reverse;
  track_node *src, *dest;
  int segmentId;
  enum direction_lock dirSegment;
  int dist;             /* in millimetres */
};

// converting sensor module/number to num
// (mod - 1) * 16 + (num - 1)

// conversions
#define SENSOR_MOD(num) (((num) >> 4) + 1) // div 16
#define SENSOR_NO(num) (((num) & 15) + 1) // mod 16
#define TRACK_NUM_FROM_SENSOR(mod, no) (((mod - 1) << 4) + (no - 1)) // mult 16

struct track_node {
  const char *name;
  node_type type;
  int num;              /* sensor or switch number */
  track_node *reverse;  /* same location, but opposite direction */
  track_edge edge[2];
};

#endif
