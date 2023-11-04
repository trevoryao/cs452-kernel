#include "deque.h"
#include "rpi.h"
#include "uassert.h"
#include "util.h"

#include "controller-consts.h"
#include "parsing.h"
#include "speed-data.h"
#include "track-data.h"

speed_data spd_data;
track_node track[TRACK_MAX];

void str_to_deque(char *in, deque *out) {
    deque_reset(out);

    for (char *itr = in; *itr != '\0'; ++itr) {
        deque_push_back(out, *itr);
    }
}

void user_main(void) {
    speed_data_init(&spd_data);
    init_track_b(track);

    deque q;
    deque_init(&q, 10);
    cmd_s cmd;

    str_to_deque("tr 77 0", &q);
    parse_cmd(&q, &cmd);
    uassert(cmd.kind == CMD_TR);
    uassert(cmd.params[0] == 77);
    uassert(cmd.params[1] == 0);
    memset(&cmd, 0, sizeof(cmd));

    str_to_deque("rv 24", &q);
    parse_cmd(&q, &cmd);
    uassert(cmd.kind == CMD_RV);
    uassert(cmd.params[0] == 24);
    memset(&cmd, 0, sizeof(cmd));

    str_to_deque("sw  14 c ", &q);
    parse_cmd(&q, &cmd);
    uassert(cmd.kind == CMD_SW);
    uassert(cmd.params[0] == 14);
    uassert(cmd.params[1] == CRV);
    memset(&cmd, 0, sizeof(cmd));

    str_to_deque("   q    ", &q);
    parse_cmd(&q, &cmd);
    uassert(cmd.kind == CMD_Q);
    memset(&cmd, 0, sizeof(cmd));

    str_to_deque("sw14 C", &q);
    parse_cmd(&q, &cmd);
    uassert(cmd.kind == ERR);
    memset(&cmd, 0, sizeof(cmd));

    str_to_deque("swag", &q);
    parse_cmd(&q, &cmd);
    uassert(cmd.kind == ERR);
    memset(&cmd, 0, sizeof(cmd));

    str_to_deque("sad", &q);
    parse_cmd(&q, &cmd);
    uassert(cmd.kind == ERR);
    memset(&cmd, 0, sizeof(cmd));

    str_to_deque("sw  0x9A C ", &q);
    parse_cmd(&q, &cmd);
    uassert(cmd.kind == CMD_SW);
    uassert(cmd.params[0] == 0x9A);
    uassert(cmd.params[1] == CRV);
    memset(&cmd, 0, sizeof(cmd));

    str_to_deque(" go   ", &q);
    parse_cmd(&q, &cmd);
    uassert(cmd.kind == CMD_GO);
    memset(&cmd, 0, sizeof(cmd));

    str_to_deque("tr 15", &q);
    parse_cmd(&q, &cmd);
    uassert(cmd.kind == ERR);
    memset(&cmd, 0, sizeof(cmd));

    /* test node parsing */
    str_to_deque("tc B16 EX7 -10 77 lo", &q);
    parse_cmd(&q, &cmd);
    uassert(cmd.kind == CMD_TC);
    uassert(!strncmp(cmd.path[0]->name, "B16", 3));
    uassert(!strncmp(cmd.path[1]->name, "EX7", 3));
    uassert(cmd.params[0] == -10);
    uassert(cmd.params[1] == 77);
    uassert(cmd.params[2] == 7);

    str_to_deque("tc MR155 E16 153 24 hi", &q);
    parse_cmd(&q, &cmd);
    uassert(cmd.kind == CMD_TC);
    uassert(!strncmp(cmd.path[0]->name, "MR155", 5));
    uassert(!strncmp(cmd.path[1]->name, "E16", 3));
    uassert(cmd.params[0] == 153);
    uassert(cmd.params[1] == 24);
    uassert(cmd.params[2] == 11);

    str_to_deque("tc EN8 E16 153 24 hi", &q);
    parse_cmd(&q, &cmd);
    uassert(cmd.kind == ERR); // EX8 doesn't exist

    str_to_deque("tc E16 BR100 0 24 hi", &q);
    parse_cmd(&q, &cmd);
    uassert(cmd.kind == ERR); // BR100 doesn't exist doesn't exist

}