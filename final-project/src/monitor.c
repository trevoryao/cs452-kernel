#include "monitor.h"

#if defined(LOG) || defined(USERLOG) || defined(KERNEL_LOG)
    #define LOGGING 1
#else
    #define LOGGING 0
#endif

// lib
#include "deque.h"
#include "sys-clock.h"
#include "term-control.h"
#include "time.h"

#if LOGGING
#include "rpi.h"
#else
#include "uart-server.h"
#endif

// usr
#include "controller-consts.h"
#include "snake.h"
#include "speed.h"
#include "speed-data.h"
#include "track-data.h"

// starting coordinates for various ui elements

#define TIME_START_X 1
#define TIME_START_Y 3

#define SW_START_X 1
#define SW_START_Y 6

#define SW_START_X 1
#define SW_START_Y 6

#define SEN_START_X 50
#define SEN_START_Y 6

#define PRMT_START_X 1
#define PRMT_START_Y 38

#define POS_START_X 1
#define POS_START_Y 30

#define NEXT_SW_PROMPT_X 14
#define NEXT_SW_PROMPT_Y 28

#define TR_START_X 5
#define TR_START_Y 32
#define TR_TAB 35

static const uint16_t SW1_START_Y = (N_SW0 / 4) + 1;

static const uint16_t SPD_START_Y = SEN_START_Y  + 10;
#define SPD_START_X SEN_START_X

static const uint16_t IDLE_START_Y = SPD_START_Y + 10;
#define IDLE_START_X 7

#define SW_TAB 10 // tab distance

extern speed_data spd_data;
extern track_node track[];

static void update_single_switch(uint16_t tid, uint16_t sw, enum SWITCH_DIR dir) {
    uint16_t x_off, y_off;
    char *fmt;

    if (SW0_BASE <= sw && sw < SW0_BASE + N_SW0) {
        x_off = ((sw - SW0_BASE) & 3) * SW_TAB; // mod 4
        y_off = (sw - SW0_BASE) >> 2; // divide by 4

        #if LOGGING
        fmt = "[switch] SW%d: %c\r\n";
        #else
        fmt = "%s" CURS_MOV "SW%d: %s%c%s";
        #endif
    } else if (SW1_BASE <= sw && sw < SW1_BASE + N_SW1) {
        x_off = ((sw - SW1_BASE) & 1) * (2 * SW_TAB); // mod 2
        y_off = ((sw - SW1_BASE) >> 1) + SW1_START_Y; // divide by 2

        #if LOGGING
        fmt = "[switch] SW%x: %c\r\n";
        #else
        fmt = "%s" CURS_MOV "SW%x: %s%c%s";
        #endif
    } else return; // error: invalid sw num

    char *col = NULL; // shut compiler up
    char dir_ch = 0; // shut compiler up

    switch (dir) { // ugly, but fast
        case UNKNOWN: col = COL_RST; dir_ch = UNKNOWN; break;
        case STRT: col = COL_MAG; dir_ch = 'S'; break;
        case CRV: col = COL_CYN; dir_ch = 'C'; break;
    }

    #if LOGGING
    (void)tid;
    (void)x_off;
    (void)y_off;
    (void)col;
    Printf(tid, fmt, sw, dir_ch);
    #else
    Printf(tid, fmt,
        CURS_SAVE,
        SW_START_Y + y_off, SW_START_X + x_off, sw, col, dir_ch,
        COL_RST CURS_UNSAVE);
    #endif
}

void init_monitor(uint16_t tid) {
    Puts(tid, CLEAR CURS_START CURS_HIDE);

    #if !LOGGING
    // paint initial structure
    // let time task start painting, no point in an initial time

    // switch list header
    Printf(tid, CURS_MOV COL_YEL "Switches:" COL_RST, SW_START_Y - 1, SW_START_X);

    for (uint16_t i = SW0_BASE; i < SW0_BASE + N_SW0; ++i) // SW0
        update_single_switch(tid, i, UNKNOWN);

    for (uint16_t i = SW1_BASE; i < SW1_BASE + N_SW1; ++i) // SW1
        update_single_switch(tid, i, GET_START_SW_STATE(i));

    // sensor list header
    // Printf(tid, CURS_MOV COL_YEL "Sensors:" COL_RST, SEN_START_Y - 1, SEN_START_X);

    // spd list header
    speed_t spd_t;
    speed_t_init(&spd_t);

    Printf(tid, CURS_MOV COL_YEL "Speed:" COL_RST, SPD_START_Y - 1, SPD_START_X);

    for (uint16_t i = 0; i < N_TRNS; ++i) {
        update_speed(tid, &spd_t, ALL_TRNS[i]);
    }

    // idle
    Printf(tid, CURS_MOV COL_YEL "Idle:" COL_RST, IDLE_START_Y, IDLE_START_X - 6);

    Printf(tid, CURS_MOV COL_YEL "Next Switch:" COL_RST, NEXT_SW_PROMPT_Y, 1);

    Printf(tid, CURS_MOV COL_YEL "Snake:" COL_RST, TR_START_Y - 2, TR_START_X - 4);
    Printf(tid, CURS_MOV "Starting Up...", TR_START_Y, TR_START_X);
    // let snake update itself

    // no triggered sensors

    #endif
}

void shutdown_monitor(uint16_t tid) {
    #if LOGGING
    (void)tid;
    #else
    Puts(tid, CLEAR CURS_START CURS_SHOW);
    #endif
}

void update_time(uint16_t tid, time_t *t) {
    #if LOGGING // don't print time if we're logging
    (void)tid;
    (void)t;
    #else
    Printf(tid, CURS_SAVE CURS_MOV DEL_LINE BLD
        "%u:%u.%u" COL_RST CURS_UNSAVE,
        TIME_START_Y, TIME_START_X, t->min, t->sec, t->tsec);
    #endif
}

void update_idle(uint16_t tid, uint64_t idle_sys_ticks, uint64_t user_sys_ticks) {
    #if LOGGING
    (void)tid;
    (void)idle_sys_ticks;
    (void)user_sys_ticks;
    #else
    time_t idle_time;
    time_from_sys_ticks(&idle_time, idle_sys_ticks);
    int idle_prop = (idle_sys_ticks * 100) / (idle_sys_ticks + user_sys_ticks);

    Printf(tid, CURS_SAVE CURS_MOV DEL_LINE BLD
        "%u:%u.%u (%d%%)"
        COL_RST CURS_UNSAVE,
        IDLE_START_Y, IDLE_START_X, idle_time.min, idle_time.sec,
        idle_time.tsec, idle_prop);
    #endif
}

void update_speed(uint16_t tid, speed_t *spd_t, uint16_t tr) {
    int8_t trn_hash_no = trn_hash(tr);
    if (trn_hash_no < 0) return;

    #if LOGGING
    (void)tid;
    Printf(tid, "[speed] TR%d: %d\r\n", tr, speed_display_get(spd_t, tr));
    #else

    char *col;
    if (speed_is_stopped(spd_t, tr)) {
        col = COL_RST;
    } else if (speed_is_rv(spd_t, tr)) {
        col = COL_RED;
    } else {
        col = COL_GRN;
    }

    Printf(tid, CURS_SAVE CURS_MOV "        " CURS_N_BWD
        "TR%d: %s%d"
        COL_RST CURS_UNSAVE,
        SPD_START_Y + trn_hash_no, SPD_START_X, 8,
        tr, col, speed_display_get(spd_t, tr));

    #endif
}

void update_switch(uint16_t tid, uint16_t sw, enum SWITCH_DIR dir) {
    update_single_switch(tid, sw, dir);

    // if SW1 switch need to throw partner in opposite dir
    if (SW1_BASE <= sw && sw < SW1_BASE + N_SW1) { // sw1?
        update_single_switch(tid, GET_SW1_PAIR(sw), (dir == STRT) ? CRV : STRT);
    }
}

void update_triggered_sensor(uint16_t tid, deque *q, uint16_t sen_mod, uint16_t sen_no) {
    if (deque_full(q)) {
        // scrolls triggered sensors
        deque_pop_back(q);
        deque_pop_back(q);
    }

    if (!deque_empty(q) &&
        deque_itr_get(q, deque_begin(q)) == sen_mod &&
        deque_itr_get(q, deque_itr_next(deque_begin(q))) == sen_no) {
        return; // drop duplicate sensor value
    }

    deque_push_front(q, sen_no);
    deque_push_front(q, sen_mod);

    #if LOGGING
    (void)tid;
    (void)q;
    Printf(tid, "[sensor trigger] %c%d\r\n", 'A' + sen_mod, sen_no);
    #else

    deque_itr it = deque_begin(q);
    for (uint16_t offset = 0; offset < (deque_size(q) >> 1); ++offset) {
        char mod = 'A' + deque_itr_get(q, it); // module number
        it = deque_itr_next(it);
        uint16_t no = deque_itr_get(q, it); // sensor number (in module)
        it = deque_itr_next(it);

        Printf(tid, CURS_SAVE CURS_MOV DEL_LINE "%c%d" CURS_UNSAVE,
            SEN_START_Y + offset, SEN_START_X,
            mod, no);
    }

    #endif
}

void init_snake_train(uint16_t tid, snake *snake, uint8_t trn_idx) {
    #if !LOGGING
    uint16_t y = TR_START_Y;
    uint16_t x = TR_START_X + (trn_idx * TR_TAB);

    uint8_t tr = snake->trns[trn_idx].trn;

    char *spd_col;
    if (speed_is_stopped(&snake->spd_t, tr)) {
        spd_col = COL_RST;
    } else if (speed_is_rv(&snake->spd_t, tr)) {
        spd_col = COL_RED;
    } else {
        spd_col = COL_GRN;
    }

    Printf(tid, CURS_SAVE CURS_MOV
        "   __________   ",
        y, x);
    Printf(tid, CURS_MOV
        "  /|  (%s%d%s)   |\\  ",
        y + 1, x, spd_col, speed_display_get(&snake->spd_t, tr), COL_RST);
    Printf(tid, CURS_MOV
        " /_|   %d   |_\\ ",
        y + 2, x, tr);
    Printf(tid, CURS_MOV
        "|              |",
        y + 3, x);
    Printf(tid, CURS_MOV
        "|______________|"
        , y + 4, x);
    Printf(tid, CURS_MOV
        "   O        O   "
        CURS_UNSAVE
        , y + 5, x);
    #endif
}

void update_snake_train(uint16_t tid, snake *snake, uint8_t trn_idx) {
    // only redraw the speed
    #if !LOGGING
    uint16_t y = TR_START_Y;
    uint16_t x = TR_START_X + (trn_idx * TR_TAB);

    uint8_t tr = snake->trns[trn_idx].trn;

    char *spd_col;
    if (speed_is_stopped(&snake->spd_t, tr)) {
        spd_col = COL_RST;
    } else if (speed_is_rv(&snake->spd_t, tr)) {
        spd_col = COL_RED;
    } else {
        spd_col = COL_GRN;
    }

    Printf(tid, CURS_SAVE CURS_MOV
        "  /|  (%s%d%s)   |\\  "
        CURS_UNSAVE,
        y + 1, x, spd_col, speed_display_get(&snake->spd_t, tr), COL_RST);
    #endif
}

void update_snake_distance(uint16_t tid, snake *snake, int16_t sen_num, uint8_t front_trn_idx, int32_t distance_mm) {
    #if LOGGING
    Printf(tid, "[snake distance] %d <- %d @ %s: %dmm\r\n",
        snake.trns[front_trn_idx].trn,
        snake.trns[front_trn_idx - 1].trn,
        track[sen_num].name, distance_mm);
    #else
    (void)snake;
    uint16_t y = TR_START_Y + 6;
    uint16_t x = TR_START_X + ((front_trn_idx - 1) * TR_TAB) + 20;

    if (distance_mm != DIST_NONE) {
        char *col;
        if ((0 <= distance_mm && distance_mm <= SMALL_FOLLOWING_DIST) ||
            distance_mm >= LARGE_FOLLOWING_DIST) {
            col = COL_RED;
        } else if (FOLLOWING_DIST_MM - FOLLOWING_DIST_MARGIN_MM <= distance_mm &&
            distance_mm <= FOLLOWING_DIST_MM + FOLLOWING_DIST_MARGIN_MM) {
            col = COL_GRN;
        } else {
            col = COL_YEL;
        }

        Printf(tid, CURS_SAVE CURS_MOV "               " CURS_N_BWD "%s%d" COL_RST "mm (%s)" CURS_UNSAVE,
            y, x, 15, col, distance_mm, track[sen_num].name);
    } else {
        Printf(tid, CURS_SAVE CURS_MOV "??" CURS_UNSAVE, y, x + 4);
    }
    #endif
}

void update_next_input_switch(uint16_t tid, uint16_t sw) {
    char *fmt;

    if (sw == 0) {
        fmt = "%s" CURS_MOV DEL_LINE "%s";
    } else if (SW0_BASE <= sw && sw < SW0_BASE + N_SW0) {
        #if LOGGING
        fmt = "[next switch] SW%d\r\n";
        #else
        fmt = "%s" CURS_MOV DEL_LINE "SW%d%s";
        #endif
    } else if (SW1_BASE <= sw && sw < SW1_BASE + N_SW1) {
        #if LOGGING
        fmt = "[next switch] SW%x\r\n";
        #else
        fmt = "%s" CURS_MOV DEL_LINE "SW%x%s";
        #endif
    } else return; // error: invalid sw num

    #if LOGGING
    Printf(tid, fmt, sw);
    #else
    Printf(tid, fmt,
        CURS_SAVE,
        NEXT_SW_PROMPT_Y, NEXT_SW_PROMPT_X, sw,
        COL_RST CURS_UNSAVE);
    #endif
}