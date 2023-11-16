#ifndef __PARSING_H__
#define __PARSING_H__

#include <stdint.h>

#include "track-node.h"

/*
 * command parsing
 */

typedef struct deque deque;

// enum codes for parsed command
enum CMD_KIND {
    ERR = 1,
    CMD_TR,
    CMD_RV,
    CMD_SW,
    CMD_TC,
    CMD_ST,
    CMD_GO,
    CMD_HLT,
    CMD_Q,
};

#define MAX_CMD_PARAMS 2

// organised multi-value return struct
typedef struct cmd_s {
    enum CMD_KIND kind;
    int16_t params[MAX_CMD_PARAMS]; // args
    track_node *path[2]; // start and end
} cmd_s;

// parses received cmd returns CMD_KIND
void parse_cmd(deque *in, cmd_s *out);

#endif
