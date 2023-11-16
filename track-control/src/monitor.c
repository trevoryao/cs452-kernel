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
#include "sensors.h"
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
#define PRMT_START_Y 30

#define POS_START_X 1
#define POS_START_Y 30

static const uint16_t SW1_START_Y = (N_SW0 / 4) + 1;

static const uint16_t SPD_START_Y = SEN_START_Y + 10;
#define SPD_START_X SEN_START_X

static const uint16_t IDLE_START_Y = SPD_START_Y + 3 * N_TRNS + 3;
#define IDLE_START_X 7

#define TC_START_Y SPD_START_Y
static const uint16_t TC_START_X = SPD_START_X + 12;

#define SW_TAB 10 // tab distance

extern speed_data spd_data;

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
    uart_printf(CONSOLE, fmt, sw, dir_ch);
    #else
    Printf(tid, fmt,
        CURS_SAVE CURS_HIDE,
        SW_START_Y + y_off, SW_START_X + x_off, sw, col, dir_ch,
        COL_RST CURS_UNSAVE CURS_SHOW);
    #endif
}

void print_prompt(uint16_t tid) {
    #if LOGGING
    (void)tid;
    #else
    Printf(tid, CURS_MOV "cmd> " DEL_LINE, PRMT_START_Y, PRMT_START_X);
    #endif
}

void init_monitor(uint16_t tid) {
    #if LOGGING
    (void)tid;
    #else
    // assume screen cleared by the kernel
    // paint initial structure
    // let time task start painting, no point in an initial time

    // switch list header
    Printf(tid, CURS_MOV COL_YEL "Switches:" COL_RST, SW_START_Y - 1, SW_START_X);

    for (uint16_t i = SW0_BASE; i < SW0_BASE + N_SW0; ++i) // SW0
        update_single_switch(tid, i, UNKNOWN);

    for (uint16_t i = SW1_BASE; i < SW1_BASE + N_SW1; ++i) // SW1
        update_single_switch(tid, i, GET_START_SW_STATE(i));

    // sensor list header
    Printf(tid, CURS_MOV COL_YEL "Sensors:" COL_RST, SEN_START_Y - 1, SEN_START_X);

    // spd list header
    speed_t spd_t;
    speed_t_init(&spd_t);

    Printf(tid, CURS_MOV COL_YEL "Speed:" COL_RST, SPD_START_Y - 1, SPD_START_X);

    for (uint16_t i = 0; i < N_TRNS; ++i) {
        update_speed(tid, &spd_t, ALL_TRNS[i]);
    }

    // train control
    Printf(tid, CURS_MOV COL_YEL "Control/Predictions:" COL_RST, TC_START_Y - 1, TC_START_X);

    // idle
    Printf(tid, CURS_MOV COL_YEL "Idle:" COL_RST, IDLE_START_Y, IDLE_START_X - 6);

    // no triggered sensors
    print_prompt(tid);

    #endif
}

void shutdown_monitor(uint16_t tid) {
    #if LOGGING
    (void)tid;
    #else
    Puts(tid, CLEAR CURS_START);
    #endif
}

void update_time(uint16_t tid, time_t *t) {
    #if LOGGING // don't print time if we're logging
    (void)tid;
    (void)t;
    #else
    Printf(tid, CURS_SAVE CURS_HIDE CURS_MOV DEL_LINE BLD
        "%u:%u.%u" COL_RST CURS_UNSAVE CURS_SHOW,
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

    Printf(tid, CURS_SAVE CURS_HIDE CURS_MOV DEL_LINE BLD
        "%u:%u.%u (%d%%)"
        COL_RST CURS_UNSAVE CURS_SHOW,
        IDLE_START_Y, IDLE_START_X, idle_time.min, idle_time.sec,
        idle_time.tsec, idle_prop);
    #endif
}

void update_speed(uint16_t tid, speed_t *spd_t, uint16_t tr) {
    int8_t trn_hash_no = trn_hash(tr);
    if (trn_hash_no < 0) return;

    #if LOGGING
    (void)tid;
    uart_printf(CONSOLE, "[speed] TR%d: %d\r\n", tr, speed_display_get(spd_t, tr));
    #else

    char *col;
    if (speed_is_stopped(spd_t, tr)) {
        col = COL_RST;
    } else if (speed_is_rv(spd_t, tr)) {
        col = COL_RED;
    } else {
        col = COL_GRN;
    }

    Printf(tid, CURS_SAVE CURS_HIDE CURS_MOV "        " CURS_N_BWD
        "TR%d: %s%d"
        COL_RST CURS_UNSAVE CURS_SHOW,
        SPD_START_Y + 3 * trn_hash_no, SPD_START_X, 8,
        tr, col, speed_display_get(spd_t, tr));

    #endif
}

void print_tc_params(uint16_t tid, int sen_track_num_start, int sen_track_num_end, int16_t offset, uint16_t trn) {
    #if LOGGING
    (void)tid;
    (void)sen_track_num_start;
    (void)sen_track_num_end;
    (void)offset;
    (void)trn;
    #else
    int8_t trn_hash_no = trn_hash(trn);
    if (trn_hash_no < 0) return;

    Printf(tid, CURS_SAVE CURS_HIDE CURS_MOV DEL_LINE
        "%c%d -> %c%d (%dmm)"
        CURS_UNSAVE CURS_SHOW,
        TC_START_Y + 3 * trn_hash_no, TC_START_X,
        SENSOR_MOD(sen_track_num_start) - 1 + 'A',
        SENSOR_NO(sen_track_num_start),
        SENSOR_MOD(sen_track_num_end) - 1 + 'A',
        SENSOR_NO(sen_track_num_end),
        offset);
#endif
}

void reset_tc_params(uint16_t tid, uint16_t trn) {
    #if LOGGING
    (void)tid;
    (void)trn;
    #else
    int8_t trn_hash_no = trn_hash(trn);
    if (trn_hash_no < 0) return;

    Printf(tid, CURS_SAVE CURS_HIDE CURS_MOV DEL_LINE
        CURS_DOWN DEL_LINE CURS_DOWN DEL_LINE
        CURS_UNSAVE CURS_SHOW,
        TC_START_Y + 3 * trn_hash_no, TC_START_X);
    #endif
}

void update_sensor_prediction(uint16_t tid, int8_t trainNo, int32_t diff) {
    // assume diff is in clock ticks (10ms)
    int32_t diff_time = diff * TICK_MS;

    #if LOGGING
    Printf(tid, "[Prediction] train %d: %dms\r\n",
        trainNo, diff_time);
    #else
    int8_t trn_hash_no = trn_hash(trainNo);
    if (trn_hash_no < 0) return;

    char *col;
    if (-5 <= diff && diff <= 5) {
        col = COL_GRN;
    } else if (-50 <= diff && diff <= 50) {
        col = COL_YEL;
    } else {
        col = COL_RED;
    }

    Printf(tid, CURS_SAVE CURS_HIDE CURS_MOV DEL_LINE
        "%s%d" COL_RST "ms"
        CURS_UNSAVE CURS_SHOW,
        TC_START_Y + 3 * trn_hash_no + 1, TC_START_X,
        col, diff_time);

    #endif
}

void print_missed_sensor(uint16_t tid, uint16_t trn, uint16_t sen_mod, uint16_t sen_no) {
    #if LOGGING
    Printf(tid, "[Prediction] train %d missed sensor %c%d\r\n",
        trn, sen_mod + 'A', sen_no + 1);
    #else
    int8_t trn_hash_no = trn_hash(trn);
    if (trn_hash_no < 0) return;

    Printf(tid, CURS_SAVE CURS_HIDE CURS_MOV DEL_LINE
        "%c%d:" COL_RED "MISSED" COL_RST
        CURS_UNSAVE CURS_SHOW,
        TC_START_Y + 3 * trn_hash_no + 2, TC_START_X,
        sen_mod + 'A', sen_no + 1);
    #endif
}

void clear_missed_sensor(uint16_t tid, uint16_t trn) {
    #if LOGGING
    (void)tid;
    (void)trn;
    #else
    int8_t trn_hash_no = trn_hash(trn);
    if (trn_hash_no < 0) return;

    Printf(tid, CURS_SAVE CURS_HIDE CURS_MOV DEL_LINE
        CURS_UNSAVE CURS_SHOW,
        TC_START_Y + 3 * trn_hash_no + 2, TC_START_X);
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
    uart_printf(CONSOLE, "[sensor trigger] %c%d\r\n", 'A' + sen_mod, sen_no);
    #else

    deque_itr it = deque_begin(q);
    for (uint16_t offset = 0; it != deque_end(q); ++offset) {
        char mod = 'A' + deque_itr_get(q, it); // module number
        it = deque_itr_next(it);
        uint16_t no = deque_itr_get(q, it); // sensor number (in module)
        it = deque_itr_next(it);

        Printf(tid, CURS_SAVE CURS_HIDE CURS_MOV DEL_LINE "%c%d" CURS_UNSAVE CURS_SHOW,
            SEN_START_Y + offset, SEN_START_X,
            mod, no);
    }

    #endif
}

void cmd_delete(uint16_t tid) {
    Puts(tid, CURS_BWD DEL_LINE);
}

void cmd_out(uint16_t tid, char c) {
    Putc(tid, c);
}

void print_error_message(uint16_t tid) {
    Printf(tid, CURS_SAVE CURS_HIDE COL_RED CURS_MOV DEL_LINE "Invalid Command" COL_RST CURS_UNSAVE CURS_SHOW,
            PRMT_START_Y + 2, PRMT_START_X);
}

void print_in_progress_message(uint16_t tid) {
    Printf(tid, CURS_SAVE CURS_HIDE COL_RED CURS_MOV DEL_LINE "Train already moving!" COL_RST CURS_UNSAVE CURS_SHOW,
            PRMT_START_Y + 2, PRMT_START_X);

}

void reset_error_message(uint16_t tid) {
    Printf(tid, CURS_SAVE CURS_HIDE CURS_MOV DEL_LINE COL_RST CURS_UNSAVE CURS_SHOW,
            PRMT_START_Y + 2, PRMT_START_X);
}

void print_train_time(uint16_t tid, int8_t trainNo, int32_t diff_time, int32_t diff_distance) {
    Printf(tid, CURS_SAVE CURS_HIDE CURS_MOV DEL_LINE "Train %d reached sensor with %d clock tick diff and %d um difference" CURS_UNSAVE CURS_SHOW,
            POS_START_Y, POS_START_X, trainNo, diff_time, diff_distance);
}