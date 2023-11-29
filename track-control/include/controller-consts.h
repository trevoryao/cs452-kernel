#ifndef __CONTROLLER_CONSTS_H__
#define __CONTROLLER_CONSTS_H__

#include <stdint.h>

#include "uart-server.h"

// constants for train speeds
#define SP_STOP 0
#define SP_REVERSE 15
#define LIGHTS 16

// constants for switch numbers
#define N_SW0 18 // switch group 0
#define SW0_BASE 1

#define N_SW1 4
#define SW1_BASE 0x99 // switch group 1

#define MAX_TRNS 100 // maximum possible train number

#define MM_TO_UM 1000 // mm -> um

// codes for switch commands
enum SWITCH_DIR {
    STRT = 33,
    CRV, // 34
    UNKNOWN = '?' // just for printing
};

static const char START_SW_STATE[4] = {STRT, CRV, CRV, STRT};
#define GET_START_SW_STATE(n) START_SW_STATE[(n) - SW1_BASE] // special preprogrammed starting state for SW1 switches

static const uint16_t SW1_PAIRS[4] = {SW1_BASE + 1, SW1_BASE, SW1_BASE + 3, SW1_BASE + 2};
#define GET_SW1_PAIR(n) SW1_PAIRS[(n) - SW1_BASE] // hardcode switch 'pairs' (can't have both C or S)

#define CUSTOM_WAIT_ARG -1 // for output buffer

#define TIME_NONE -1
#define DIST_NONE -1

// time consts
#define RV_WAIT_TIME    320 // 3.2s
#define REFRESH_TIME    9   // refresh clock every 90ms
#define IDLE_REFRESH_TIME 500 // 5s

#define NUM_MOD_PER_SEN 16

#endif
