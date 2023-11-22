#include "preset-program.h"

#include "clock.h"
#include "task.h"
#include "uart-server.h"

#include "monitor.h"
#include "track-control.h"
#include "track-data.h"
#include "track-segment-locking.h"
#include "train.h"

extern track_node track[];

void init_preset_program(preset_program *program) {
    /***** test 0 *****/
    program->presets[0].num_of_trns = 2;

    // trn 24
    program->presets[0].trns[0].trn = 24;
    program->presets[0].trns[0].start = &track[0];
    program->presets[0].trns[0].end = &track[78];
    program->presets[0].trns[0].spd = SPD_LO;

    // trn 58
    program->presets[0].trns[1].trn = 58;
    program->presets[0].trns[1].start = &track[16];
    program->presets[0].trns[1].end = &track[14];
    program->presets[0].trns[1].spd = SPD_LO;

    /***** Ken's Test (1 & 2) *****/
    program->presets[1].num_of_trns = 2;
    program->presets[2].num_of_trns = 2;

    // trn 24
    program->presets[1].trns[0].trn = 77;
    program->presets[1].trns[0].start = &track[58]; // D11
    program->presets[1].trns[0].end = &track[71]; // E8
    program->presets[1].trns[0].spd = SPD_LO;

    // trn 58
    program->presets[1].trns[1].trn = 24;
    program->presets[1].trns[1].start = &track[38]; // C7
    program->presets[1].trns[1].end = &track[2]; // A3
    program->presets[1].trns[1].spd = SPD_LO;

    // trn 24
    program->presets[2].trns[0].trn = 77;
    program->presets[2].trns[0].start = &track[71]; // E8
    program->presets[2].trns[0].end = &track[58]; // D11
    program->presets[2].trns[0].spd = SPD_LO;

    // trn 58
    program->presets[2].trns[1].trn = 24;
    program->presets[2].trns[1].start = &track[2]; // A3
    program->presets[2].trns[1].end = &track[38]; // C7
    program->presets[2].trns[1].spd = SPD_LO;

    // triple insanity (3)
    program->presets[3].num_of_trns = 2;

    // trn 24
    program->presets[3].trns[0].trn = 24;
    program->presets[3].trns[0].start = &track[0]; // A1
    program->presets[3].trns[0].end = &track[15]; // A16
    program->presets[3].trns[0].spd = SPD_LO;

    // trn 58
    program->presets[3].trns[1].trn = 77;
    program->presets[3].trns[1].start = &track[15]; // A16
    program->presets[3].trns[1].end = &track[79]; // E16
    program->presets[3].trns[1].spd = SPD_LO;

    // trn 77
    // program->presets[3].trns[1].trn = 77;
    // program->presets[3].trns[1].start = &track[79]; // E16
    // program->presets[3].trns[1].end = &track[0]; // A1
    // program->presets[3].trns[1].spd = SPD_MED;
}

// see return codes
#define PRESET_EXIT -1
#define PRESET_BACK 0
#define PRESET_FWD 1
#define PRESET_REDO 2
static int preset_wait_loop(int console_tid) {

    for (;;) {
        switch (Getc(console_tid)) {
            case 'n': return PRESET_FWD;
            case 'c': return PRESET_REDO;
            case 'b': return PRESET_BACK;
            case 'q': return PRESET_EXIT;
            default: break; // continue
        }
    }
}

void run_preset_program(preset_program *program, uint8_t itr,
    uint16_t trains[], int console_tid, int tcc_tid, int ts_tid, int clock_tid) {
    for (;;) {
        // create each train
        for (int j = 0; j < program->presets[itr].num_of_trns; ++j) {
            trains[trn_hash(program->presets[itr].trns[j].trn)] = CreateControlledTrain(
                program->presets[itr].trns[j].trn,
                program->presets[itr].trns[j].start,
                program->presets[itr].trns[j].end,
                0,
                program->presets[itr].trns[j].spd
            );
            print_tc_params(
                console_tid,
                program->presets[itr].trns[j].start->num,
                program->presets[itr].trns[j].end->num,
                0,
                program->presets[itr].trns[j].trn
            );

            Delay(clock_tid, 50);
        }

        uint8_t last = itr;

        // wait for next cmd
        switch (preset_wait_loop(console_tid)) {
            case PRESET_EXIT: return;
            case PRESET_BACK:
                itr = (itr + N_PRESET_PROGRAMS - 1) % N_PRESET_PROGRAMS;
                break;
            case PRESET_FWD:
                itr = (itr + 1) % N_PRESET_PROGRAMS;
                break;
            case PRESET_REDO: break;
        }

        // assume stopped
        for (int j = 0; j < program->presets[last].num_of_trns; ++j) {
            uint8_t trn = program->presets[last].trns[j].trn;

            // call st for each train
            KillChild(trains[trn_hash(trn)]);
            track_control_end_train(tcc_tid, trn); // deregister on behalf of killed train
            track_control_set_train_speed(tcc_tid, trn, SPD_STP);
            track_server_free_all(ts_tid, -1, trn);
        }
    }
}
