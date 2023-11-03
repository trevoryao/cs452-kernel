#include "parsing.h"

#include "deque.h"

#include "controller-consts.h"
#include "speed.h"

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
                else if (tr_no >= MAX_TRNS) { RET_ERR } // invalid
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
                out->kind = CMD_TC;

                for (int i = 0; i < 2; ++i) {
                    if (strip_ws(in) == 0) { RET_ERR } // no ws?

                    char mod = (char)deque_pop_front(in);
                    if ('A' <= mod && mod <= 'E') {
                        out->path[i].mod_num = mod - 'A' + 1;
                    } else if ('a' <= mod && mod <= 'e') {
                        out->path[i].mod_num = mod - 'a' + 1;
                    } else { RET_ERR }

                    int mod_no;
                    if ((mod_no = parse_num(in)) < 0) { RET_ERR }
                    else out->path[i].mod_sensor = mod_no;

                    out->path[i].num = (mod - 1) * NUM_MOD_PER_SEN + (mod_no - 1);
                }

                if (strip_ws(in) == 0) { RET_ERR } // no ws?

                int mult = (deque_front(in) == '-') ? -1 : 1;
                int offset; // train no
                if ((offset = parse_num(in)) < 0) { RET_ERR } // not a num
                out->params[0] = mult * offset;

                if (strip_ws(in) == 0) { RET_ERR } // no ws?

                // get desired spd
                c = (char)deque_pop_front(in);
                if (c == 'l') {
                    if (deque_pop_front(in) == 'o') {
                        out->params[1] = SPD_LO;
                    } else { RET_ERR }
                } else if (c == 'm') {
                    if (deque_pop_front(in) == 'e' &&
                        deque_pop_front(in) == 'd') {
                        out->params[1] = SPD_MED;
                    } else { RET_ERR }
                } else if (c == 'h') {
                    if (deque_pop_front(in) == 'i') {
                        out->params[1] = SPD_HI;
                    } else { RET_ERR }
                } else { RET_ERR }

                break;
            } else { RET_ERR }
        }
        case 'r': {
            if ((char)deque_pop_front(in) == 'v') { // valid cmd?
                // get train no
                if (strip_ws(in) == 0) { RET_ERR } // no ws?
                int tr_no; // train no
                if ((tr_no = parse_num(in)) < 0) { RET_ERR } // not a num
                else if (tr_no >= MAX_TRNS) { RET_ERR } // invalid
                out->params[0] = tr_no;

                out->kind = CMD_RV;
                break; // check end
            } else { RET_ERR }
        }
        case 's': {
            if ((char)deque_pop_front(in) == 'w') { // valid cmd?
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
            } else { RET_ERR }
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
