#include "parsing.h"

#include "deque.h"

#include "controller-consts.h"
#include "speed.h"

extern track_node track[];

// returns number of ws chars stripped
static uint16_t strip_ws(deque *in) {
    char c;
    uint16_t n = 0;
    while (!deque_empty(in)) {
        c = (char)deque_front(in);
        if (!(c == ' ' || c == '\t')) break; // EXIT: not whitespace
        deque_pop_front(in);
        ++n;
    }

    return n;
}

// pos only, returns -1 on error
static int parse_int(deque *in) {
    deque stack; // int stack (not ascii)
    deque_init(&stack, 7); // 128

    // parse
    char c;
    while (!deque_empty(in)) {
        c = (char)deque_front(in);

        if ('0' <= c && c <= '9') {
            deque_push_back(&stack, c - '0');
        } else break; // EXIT: not digit

        deque_pop_front(in);
    }

    // calculate value
    int res = 0;
    for (uint32_t dig = 1; !deque_empty(&stack); dig *= 10)
        res += dig * deque_pop_back(&stack);

    return res;
}

static int parse_hex_int(deque *in) {
    deque stack; // int stack (not ascii)
    deque_init(&stack, 7); // 128

    // parse
    char c;
    while (!deque_empty(in)) {
        c = (char)deque_front(in);

        if ('0' <= c && c <= '9') {
            deque_push_back(&stack, c - '0');
        } else if ('a' <= c && c <= 'f') {
            deque_push_back(&stack, c - 'a' + 10);
        } else if ('A' <= c && c <= 'F') {
            deque_push_back(&stack, c - 'A' + 10);
        } else break; // EXIT: not digit

        deque_pop_front(in);
    }

    // calculate value
    int res = 0;
    for (uint32_t dig = 1; !deque_empty(&stack); dig *= 16)
        res += dig * deque_pop_back(&stack);

    return res;
}

// pos only, returns -1 on error
static int parse_num(deque *in) {
    if (deque_front(in) == '0') {
        deque_pop_front(in);
        if (deque_front(in) == 'x' || deque_front(in) == 'X') {
            deque_pop_front(in);
            return parse_hex_int(in);
        } else {
            return parse_int(in);
        }
    } else return parse_int(in);
}

#define SENS_NODE_BASE  0
#define SW0_NODE_BASE   80
#define SW1_NODE_BASE   116
#define END_NODE_BASE   124
#define END1_NODE_BASE  132

/*
 * [A-E][1-16]
 * (B/M)R[1-18 / 153 - 156]
 * E(N/X)[1-10]
 */

static int parse_path_node(deque *in, track_node **out) {
    char second_char = deque_itr_get(in, deque_itr_next(deque_begin(in)));
    switch (second_char) { // check second char to eliminate many
        case 'R': { // BR or MR?
            int offset;
            char c = (char)deque_pop_front(in);
            if (c == 'B') {
                offset = 0;
            } else if (c == 'M') {
                offset = 1;
            } else return -1;

            deque_pop_front(in); // pop off all letters

            int sw_no = parse_num(in);
            if (sw_no < 0) return -1;
            else if (SW0_BASE <= sw_no && sw_no < SW0_BASE + N_SW0) {
                *out = &track[SW0_NODE_BASE + (2 * (sw_no - SW0_BASE)) + offset];
            } else if (SW1_BASE <= sw_no && sw_no < SW1_BASE + N_SW1) {
                *out = &track[SW1_NODE_BASE + (2 * (sw_no - SW1_BASE)) + offset];
            } else {
                return -1;
            }

            break;
        }
        case 'N': // EN?
        case 'X': { // EX?
            if ((char)deque_pop_front(in) != 'E') {
                return -1;
            }

            int offset = (second_char == 'N') ? 0 : 1; // offset 1 for EX

            deque_pop_front(in); // pop off all letters

            // switch number parsing super ugly since not very linear
            int end_no = parse_num(in);
            if (end_no < 0) return -1;
            else if (1 <= end_no && end_no <= 5) {
                *out = &track[END_NODE_BASE + (2 * (end_no - 1)) + offset];
            } else if (5 <= end_no && end_no <= 9 && ((end_no % 2) == 1)) {
                *out = &track[END1_NODE_BASE + (end_no - 5) + offset];
            } else if (end_no == 10) {
                *out = &track[END1_NODE_BASE + 4 + offset];
            } else {
                return -1;
            }

            break;
        }
        default: { // try parse as switch
            char mod = (char)deque_pop_front(in);
            if ('A' <= mod && mod <= 'E') {
                mod = mod - 'A';
            } else return -1;

            int mod_no = parse_num(in);
            if (mod_no < 0) return -1;
            else if (1 <= mod_no && mod_no <= NUM_MOD_PER_SEN) {
                *out = &track[SENS_NODE_BASE + (mod * NUM_MOD_PER_SEN) + (mod_no - 1)];
            } else {
                return -1;
            }

            break;
        }
    }

    return 0;
}

#define RET_ERR out->kind = ERR; return; // just to stop repetition, not amazing form

void parse_cmd(struct deque *in, cmd_s *out) {
    strip_ws(in);

    char c = (char)deque_pop_front(in);
    switch (c) {
        case 't': {
            c = (char)deque_pop_front(in);
            if (c == 'r') { // valid cmd?
                // get train no
                if (strip_ws(in) == 0) { RET_ERR } // no ws?
                int tr_no; // train no
                if ((tr_no = parse_num(in)) < 0) { RET_ERR } // not a num
                else if (trn_hash(tr_no) == -1) { RET_ERR } // invalid
                out->params[0] = tr_no;

                // get train speed
                if (strip_ws(in) == 0) { RET_ERR } // no ws?
                int spd; // train no
                if ((spd = parse_num(in)) < 0) { RET_ERR } // not a num
                else if (spd > SP_REVERSE + LIGHTS) { RET_ERR } // invalid
                out->params[1] = spd;

                out->kind = CMD_TR;
                break; // check end
            } else if (c == 'c') {
                for (int i = 0; i < 2; ++i) {
                    if (strip_ws(in) == 0) { RET_ERR } // no ws?
                    if (parse_path_node(in, &out->path[i]) < 0) { RET_ERR }
                }

                // parse offset
                if (strip_ws(in) == 0) { RET_ERR } // no ws?

                int mult = 1;
                if (deque_front(in) == '-') {
                    mult = -1;
                    deque_pop_front(in);
                }

                int offset;
                if ((offset = parse_num(in)) < 0) { RET_ERR } // not a num
                out->params[0] = mult * offset;

                // parse train number
                if (strip_ws(in) == 0) { RET_ERR } // no ws?
                int tr_no; // train no
                if ((tr_no = parse_num(in)) < 0) { RET_ERR } // not a num
                else if (trn_hash(tr_no) == -1) { RET_ERR } // invalid
                out->params[1] = tr_no;

                out->kind = CMD_TC;
                break;
            } else { RET_ERR }
        }
        case 'r': {
            if ((char)deque_pop_front(in) == 'v') { // valid cmd?
                // get train no
                if (strip_ws(in) == 0) { RET_ERR } // no ws?
                int tr_no; // train no
                if ((tr_no = parse_num(in)) < 0) { RET_ERR } // not a num
                else if (trn_hash(tr_no) == -1) { RET_ERR } // invalid
                out->params[0] = tr_no;

                out->kind = CMD_RV;
                break; // check end
            } else { RET_ERR }
        }
        case 's': {
            c = (char)deque_pop_front(in);
            if (c == 'w') { // sw?
                // get switch no
                if (strip_ws(in) == 0) { RET_ERR } // no ws?
                int sw; // switch no
                if ((sw = parse_num(in)) < 0) { RET_ERR } // not a num

                out->params[0] = sw;

                if (!(SW0_BASE <= sw && sw < SW0_BASE + N_SW0) &&
                    !(SW1_BASE <= sw && sw < SW1_BASE + N_SW1)) { // invalid?
                    RET_ERR
                }

                // get dir
                if (strip_ws(in) == 0) { RET_ERR } // no ws?

                char dir = (char)deque_pop_front(in);
                switch (dir) {
                    case 'S':
                    case 's':
                        out->params[1] = STRT;
                        break;
                    case 'C':
                    case 'c':
                        out->params[1] = CRV;
                        break;
                    default: { RET_ERR }
                }

                out->kind = CMD_SW;
                break; // check end
            } else if (c == 't') { // st?
                // parse train number
                if (strip_ws(in) == 0) { RET_ERR } // no ws?
                int tr_no; // train no
                if ((tr_no = parse_num(in)) < 0) { RET_ERR } // not a num
                else if (trn_hash(tr_no) == -1) { RET_ERR } // invalid
                out->params[0] = tr_no;

                out->kind = CMD_ST;
                break;
            }
            else { RET_ERR }
        }
        case 'g': {
            if ((char)deque_pop_front(in) == 'o') {
                out->kind = CMD_GO;
                break;
            } else { RET_ERR }
        }
        case 'h': {
            if ((char)deque_pop_front(in) == 'l' &&
                (char)deque_pop_front(in) == 't') {
                out->kind = CMD_HLT;
                break;
            } else { RET_ERR }
        }
        case 'q': {
            out->kind = CMD_Q;
            break; // check end
        }
        default: { RET_ERR }
    }

    // make sure nothing else after parsing
    strip_ws(in);
    if (!deque_empty(in)) { RET_ERR } // extra after?
}
