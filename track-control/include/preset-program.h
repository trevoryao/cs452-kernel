#ifndef __PRESET_PROGRAM__
#define __PRESET_PROGRAM__

#include "speed.h"

typedef struct track_node track_node;

#define N_PRESET_PROGRAMS 4

typedef struct train_instrs {
    uint8_t trn;
    track_node *start, *end;
    uint8_t spd;
} train_instrs;

typedef struct preset_program_entry {
    uint8_t num_of_trns;
    train_instrs trns[N_TRNS];
} preset_program_entry;

typedef struct preset_program {
    preset_program_entry presets[N_PRESET_PROGRAMS];
} preset_program;

void init_preset_program(preset_program *program);
void run_preset_program(preset_program *program, uint8_t itr,
    uint16_t trains[], int console_tid, int tcc_tid, int ts_tid, int clock_tid);

#endif