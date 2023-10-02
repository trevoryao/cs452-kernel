#include "k2.h"

// lib
#include "msg.h"
#include "rpi.h"
#include "task.h"
#include "uassert.h"
#include "util.h"

// user
#include "timer-stats.h"

static const int N_TIMING_MSG_SIZES = 3;
static const int TIMING_MSG_SIZES[] = {4, 64, 256};

static const int N_TIMINGS = 1000; // for average

struct timing_msg {
    int size;
    uint16_t pair_tid;
    int send_first;
};

static timer_stats stats;

void kernel2_test() {
    #if defined(K2_TEST_GAME)
    run_game();
    #elif defined(K2_TEST_TIMINGS)
    run_timings();
    #endif
}

static void sender(void) {
    ULOG("sender -- start\r\n");

    int p_tid;
    struct timing_msg info_msg;
    Receive(&p_tid, (char *)&info_msg, sizeof(struct timing_msg));
    ULOG("sender -- received info from parent\r\n");
    Reply(p_tid, NULL, 0); // unblock parent

    // construct msg
    char msg[TIMING_MSG_SIZES[N_TIMING_MSG_SIZES - 1] + 1];
    memset(msg, 'A', info_msg.size);

    char reply[1] = {0};

    // start send
    if (info_msg.send_first) {
        timer_stats_start(&stats, PERF_TMR);
    }

    ULOG("sender -- sending message\r\n");
    int reply_len = Send(info_msg.pair_tid, msg, info_msg.size, reply, 1);
    ULOG("sender -- received message\r\n");

    timer_stats_end(&stats, PERF_TMR);

    uassert(reply_len == 1);
    uassert(reply[0] = 'A');

    ULOG("sender -- done\r\n");
    Send(p_tid, NULL, 0, NULL, 0);
}

static void receiver() {
    ULOG("rcver -- start\r\n");

    int p_tid;
    struct timing_msg info_msg;
    Receive(&p_tid, (char *)&info_msg, sizeof(struct timing_msg));
    ULOG("rcver -- received info from parent\r\n");
    Reply(p_tid, NULL, 0); // unblock parent

    char msg[TIMING_MSG_SIZES[N_TIMING_MSG_SIZES - 1] + 1];
    int sender_tid;

    if (!info_msg.send_first) {
        timer_stats_start(&stats, PERF_TMR);
    }

    ULOG("rcver -- waiting for msg from sender\r\n");
    int rcv_len = Receive(&sender_tid, msg, info_msg.size);
    ULOG("rcver -- received msg from sender\r\n");

    msg[info_msg.size] = '\0'; // null terminator dropped

    uassert(rcv_len == info_msg.size);
    uassert(sender_tid == info_msg.pair_tid);

    // construct duplicate msg
    char dup_msg[TIMING_MSG_SIZES[N_TIMING_MSG_SIZES - 1] + 1];
    memset(dup_msg, 'A', info_msg.size);
    dup_msg[info_msg.size] = '\0';

    uassert(!strncmp(msg, dup_msg, TIMING_MSG_SIZES[N_TIMING_MSG_SIZES - 1]));

    ULOG("rcver -- replying to sender\r\n");
    int reply_len = Reply(sender_tid, msg, info_msg.size);

    uassert(reply_len == 1);

    ULOG("rcver -- done\r\n");

    Send(p_tid, NULL, 0, NULL, 0);
}

void run_timings() {
    timer_stats_init(&stats);

    uint16_t my_tid = MyTid();

    for (int i = 0; i < N_TIMING_MSG_SIZES; ++i) {
        uart_printf(CONSOLE, "Testing Message size %d\r\n", TIMING_MSG_SIZES[i]);
        uart_printf(CONSOLE, "SENDER FIRST\r\n");

        for (int j = 0; j < N_TIMINGS; ++j) {
            uint16_t send_tid = Create(P_MED, sender);
            uint16_t rcv_tid = Create(P_MED, receiver);

            ULOG("Runner TID: %d Sender TID: %d Receiver TID: %d\r\n", my_tid, send_tid, rcv_tid);

            struct timing_msg send_msg = {TIMING_MSG_SIZES[i], rcv_tid, 1};
            struct timing_msg rcv_msg = {TIMING_MSG_SIZES[i], send_tid, 1};

            Send(send_tid, (char *)&send_msg, sizeof(struct timing_msg), NULL, 0); // start sender first
            ULOG("sender task unblocked runner\r\n");
            Send(rcv_tid, (char *)&rcv_msg, sizeof(struct timing_msg), NULL, 0); // receiver
            ULOG("rcver task unblocked runner\r\n");

            // block until both finished
            int unblock_tid;
            // block until both finished
            Receive(&unblock_tid, NULL, 0);
            ULOG("runner got unblocked\r\n");
            Reply(unblock_tid, NULL, 0);

            Receive(&unblock_tid, NULL, 0);
            ULOG("runner got unblocked\r\n");
            Reply(unblock_tid, NULL, 0);
        }

        uart_printf(CONSOLE, "Results:\r\n");
        timer_stats_print_avg(&stats, PERF_TMR);
        timer_stats_print_max(&stats, PERF_TMR);

        timer_stats_clear(&stats, PERF_TMR);

        uart_printf(CONSOLE, "RECEIVER FIRST\r\n");

        for (int j = 0; j < N_TIMINGS; ++j) {
            uint16_t send_tid = Create(P_MED, sender);
            uint16_t rcv_tid = Create(P_MED, receiver);

            ULOG("Runner TID: %d Sender TID: %d Receiver TID: %d\r\n", my_tid, send_tid, rcv_tid);

            struct timing_msg send_msg = {TIMING_MSG_SIZES[i], rcv_tid, 0};
            struct timing_msg rcv_msg = {TIMING_MSG_SIZES[i], send_tid, 0};

            Send(rcv_tid, (char *)&rcv_msg, sizeof(struct timing_msg), NULL, 0); // receiver first
            ULOG("rcver task unblocked runner\r\n");
            Send(send_tid, (char *)&send_msg, sizeof(struct timing_msg), NULL, 0); // sender
            ULOG("sender task unblocked runner\r\n");


            // block until both finished
            int unblock_tid;
            // block until both finished
            Receive(&unblock_tid, NULL, 0);
            ULOG("runner got unblocked\r\n");
            Reply(unblock_tid, NULL, 0);

            Receive(&unblock_tid, NULL, 0);
            ULOG("runner got unblocked\r\n");
            Reply(unblock_tid, NULL, 0);
        }

        uart_printf(CONSOLE, "Results:\r\n");
        timer_stats_print_avg(&stats, PERF_TMR);
        timer_stats_print_max(&stats, PERF_TMR);

        timer_stats_clear(&stats, PERF_TMR);
    }
}
