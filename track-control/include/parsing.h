#ifndef __PARSING_H__
#define __PARSING_H__

#include <stdint.h>

#include "sensors.h"

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
    CMD_GO,
    CMD_HLT,
    CMD_Q,
};

// organised multi-value return struct
typedef struct cmd_s {
    enum CMD_KIND kind;
    union {
        uint16_t params[2]; // args
        sensor path[2]; // start and end
    } args;
} cmd_s;

// parses received cmd returns CMD_KIND
void parse_cmd(deque *in, cmd_s *out);

#endif
