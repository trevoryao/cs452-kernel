#include "k2.h"

// lib
#include "msg.h"
#include "rpi.h"
#include "task.h"
#include "uassert.h"
#include "util.h"

// user
#include "timer-stats.h"
#include "gameserver.h"
#include "nameserver.h"

static const int N_TIMING_MSG_SIZES = 3;
static const int TIMING_MSG_SIZES[] = {4, 64, 256};

static const int N_TIMINGS = 1000; // for average

struct timing_msg {
    int size;
    uint16_t pair_tid;
    int send_first;
};

struct game_msg {
    char player_name[256];
    int n_of_moves;
    enum GAME_TURN move[10];
    enum GAME_MSG_TYPE expected_outcome[10];
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

    char reply[TIMING_MSG_SIZES[N_TIMING_MSG_SIZES - 1] + 1];

    // start send
    if (info_msg.send_first) {
        timer_stats_start(&stats, PERF_TMR);
    }

    ULOG("sender -- sending message\r\n");
    int reply_len = Send(info_msg.pair_tid, msg, info_msg.size, reply, info_msg.size);
    ULOG("sender -- received message\r\n");

    timer_stats_end(&stats, PERF_TMR);

    // uassert(reply_len == 1);
    // uassert(reply[0] = 'A');

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

    // uassert(rcv_len == info_msg.size);
    // uassert(sender_tid == info_msg.pair_tid);

    // construct duplicate msg
    // char dup_msg[TIMING_MSG_SIZES[N_TIMING_MSG_SIZES - 1] + 1];
    // memset(dup_msg, 'A', info_msg.size);
    // dup_msg[info_msg.size] = '\0';

    // uassert(!strncmp(msg, dup_msg, TIMING_MSG_SIZES[N_TIMING_MSG_SIZES - 1]));

    ULOG("rcver -- replying to sender\r\n");
    int reply_len = Reply(sender_tid, msg, info_msg.size);

    // uassert(reply_len == 1);

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


#define CLIENT_LOG(fmt, ...) uart_printf(CONSOLE, COL_GRN "[Game Client] " fmt COL_RST "\r\n" __VA_OPT__(,) __VA_ARGS__)
#define CLIENT_LOG_ERROR(fmt, ...) uart_printf(CONSOLE, COL_RED "[Game Client ERROR] " fmt COL_RST "\r\n" __VA_OPT__(,) __VA_ARGS__)
#define TEST_LOG(fmt, ...) uart_printf(CONSOLE, COL_WHT fmt COL_RST "\r\n" __VA_OPT__(,) __VA_ARGS__)


void generic_client(void) {
    // get the information from main routine
    struct game_msg data;
    int senderTid = 0;
    uint16_t client_tid = MyTid();
    Receive(&senderTid, (char *)&data, sizeof(struct game_msg));
    // reply to main loop
    Reply(senderTid, NULL, 0);

    CLIENT_LOG("%s started with tid %d", data.player_name, client_tid);

    uint16_t gameserverTid = WhoIs("gameserver");
    uint16_t myTid = MyTid();

    msg_gameserver request;
    msg_gameserver reply;

    request.playerTid = myTid;
    request.type = MSG_GAMESERVER_SIGNUP;


    CLIENT_LOG("%s tries to register with gameserver", data.player_name);
    Send(gameserverTid, (char *)&request, sizeof(msg_gameserver), (char *)&reply, sizeof(msg_gameserver));

    if (reply.type == MSG_GAMESERVER_MOVE_REQUIRED) {
        CLIENT_LOG("%s successfully registered", data.player_name);
    } else {
        CLIENT_LOG("%s got unexpected message", data.player_name);
    }

    int i = 0;
    for (i = 0; i < data.n_of_moves; i++) {
        CLIENT_LOG("%s plays %s", data.player_name, GAME_TURN_NAMES[data.move[i]]);

        request.type = MSG_GAMESERVER_PLAY;
        request.gameTurn = data.move[i];
        Send(gameserverTid, (char *)&request, sizeof(msg_gameserver), (char *)&reply, sizeof(msg_gameserver));

        CLIENT_LOG("%s got response: %s", data.player_name, GAME_MSG_NAMES[reply.type - N_MSG_TYPE]);
        if (reply.type != data.expected_outcome[i]) {
            CLIENT_LOG("%s last response was unexpected", data.player_name);
        } else if (data.expected_outcome[i] == MSG_GAMESERVER_QUIT) {
            int parent_tid = MyParentTid();
            Send(parent_tid, NULL, 0, NULL, 0);
            return;
        }
    }


    // quit if no other command
    CLIENT_LOG("%s quits", data.player_name);
    request.type = MSG_GAMESERVER_QUIT;
    Send(gameserverTid, (char *)&request, sizeof(msg_gameserver), (char *)&reply, sizeof(msg_gameserver));

    // Set parent that I'm done
    int parent_tid = MyParentTid();
    Send(parent_tid, NULL, 0, NULL, 0);
}


void wait_for_next_test() {
    TEST_LOG("----Press any key to continue----");
    uart_getc(CONSOLE);
}

/*
 * test1: check tie
 * test2: check rock win
 * test3: check paper win
 * test4: check scissors win
 * test5: multiple rounds
 * test6: early quit
 * test7: multiple clients waiting
 */

void run_game(void) {
    // start up game server
    Create(P_SERVER_LO, gameserver_main);
    int tid_1, tid_2, tid_3, tid_4;
    int waiting_tid;
    struct game_msg player1, player2, player3, player4;

    strncpy(player1.player_name, "Player 1", 9);
    strncpy(player2.player_name, "Player 2", 9);
    strncpy(player3.player_name, "Player 3", 9);
    strncpy(player4.player_name, "Player 4", 9);

    // Test 1: Two identical player with a tie
    TEST_LOG("-------Starting Test 1: Expected Tie-------");
    player1.move[0] = GAME_PAPER;
    player2.move[0] = GAME_PAPER;
    player1.expected_outcome[0] = MSG_GAMESERVER_TIE;
    player2.expected_outcome[0] = MSG_GAMESERVER_TIE;
    player1.n_of_moves = 1;
    player2.n_of_moves = 1;

    tid_1 = Create(P_MED, generic_client);
    Send(tid_1, (char *)&player1, sizeof(struct game_msg), NULL, 0);
    tid_2 = Create(P_MED, generic_client);
    Send(tid_2, (char *)&player2, sizeof(struct game_msg), NULL, 0);

    Receive(&waiting_tid, NULL, 0); // waiting for processes to finish
    Reply(waiting_tid, NULL, 0);
    Receive(&waiting_tid, NULL, 0);
    Reply(waiting_tid, NULL, 0);

    wait_for_next_test();

    // Test 2: check rock win
    TEST_LOG("-------Starting Test 2: Expected Player 1 wins with rock-------");
    player1.move[0] = GAME_ROCK;
    player2.move[0] = GAME_SCISSORS;
    player1.expected_outcome[0] = MSG_GAMESERVER_WIN;
    player2.expected_outcome[0] = MSG_GAMESERVER_LOSE;
    player1.n_of_moves = 1;
    player2.n_of_moves = 1;

    tid_1 = Create(P_MED, generic_client);
    Send(tid_1, (char *)&player1, sizeof(struct game_msg), NULL, 0);
    tid_2 = Create(P_MED, generic_client);
    Send(tid_2, (char *)&player2, sizeof(struct game_msg), NULL, 0);

    Receive(&waiting_tid, NULL, 0); // waiting for processes to finish
    Reply(waiting_tid, NULL, 0);
    Receive(&waiting_tid, NULL, 0);
    Reply(waiting_tid, NULL, 0);

    wait_for_next_test();


    // Test 3: check paper win
    TEST_LOG("-------Starting Test 3: Expected Player 2 wins with paper-------");
    player1.move[0] = GAME_ROCK;
    player2.move[0] = GAME_PAPER;
    player1.expected_outcome[0] = MSG_GAMESERVER_LOSE;
    player2.expected_outcome[0] = MSG_GAMESERVER_WIN;
    player1.n_of_moves = 1;
    player2.n_of_moves = 1;

    tid_1 = Create(P_MED, generic_client);
    Send(tid_1, (char *)&player1, sizeof(struct game_msg), NULL, 0);
    tid_2 = Create(P_MED, generic_client);
    Send(tid_2, (char *)&player2, sizeof(struct game_msg), NULL, 0);

    Receive(&waiting_tid, NULL, 0); // waiting for processes to finish
    Reply(waiting_tid, NULL, 0);
    Receive(&waiting_tid, NULL, 0);
    Reply(waiting_tid, NULL, 0);

    wait_for_next_test();


    // Test 4: check scissors win
    TEST_LOG("-------Starting Test 4: Expected Player 1 wins with scissors-------");
    player1.move[0] = GAME_SCISSORS;
    player2.move[0] = GAME_PAPER;
    player1.expected_outcome[0] = MSG_GAMESERVER_WIN;
    player2.expected_outcome[0] = MSG_GAMESERVER_LOSE;
    player1.n_of_moves = 1;
    player2.n_of_moves = 1;

    tid_1 = Create(P_MED, generic_client);
    Send(tid_1, (char *)&player1, sizeof(struct game_msg), NULL, 0);
    tid_2 = Create(P_MED, generic_client);
    Send(tid_2, (char *)&player2, sizeof(struct game_msg), NULL, 0);

    Receive(&waiting_tid, NULL, 0); // waiting for processes to finish
    Reply(waiting_tid, NULL, 0);
    Receive(&waiting_tid, NULL, 0);
    Reply(waiting_tid, NULL, 0);

    wait_for_next_test();

    // Test 5: multiple rounds
    TEST_LOG("-------Starting Test 5: Expected Player 1 wins first round, Player 2 second round-------");
    player1.move[0] = GAME_SCISSORS;
    player1.move[1] = GAME_SCISSORS;
    player2.move[0] = GAME_PAPER;
    player2.move[1] = GAME_ROCK;
    player1.expected_outcome[0] = MSG_GAMESERVER_WIN;
    player1.expected_outcome[1] = MSG_GAMESERVER_LOSE;
    player2.expected_outcome[0] = MSG_GAMESERVER_LOSE;
    player2.expected_outcome[1] = MSG_GAMESERVER_WIN;
    player1.n_of_moves = 2;
    player2.n_of_moves = 2;

    tid_1 = Create(P_MED, generic_client);
    Send(tid_1, (char *)&player1, sizeof(struct game_msg), NULL, 0);
    tid_2 = Create(P_MED, generic_client);
    Send(tid_2, (char *)&player2, sizeof(struct game_msg), NULL, 0);

    Receive(&waiting_tid, NULL, 0); // waiting for processes to finish
    Reply(waiting_tid, NULL, 0);
    Receive(&waiting_tid, NULL, 0);
    Reply(waiting_tid, NULL, 0);

    wait_for_next_test();


    // Test 6: early quitting
    TEST_LOG("-------Starting Test 6: Expected Player 1 wins first round and quits early-------");
    player1.move[0] = GAME_SCISSORS;
    player2.move[0] = GAME_PAPER;
    player2.move[1] = GAME_ROCK;
    player1.expected_outcome[0] = MSG_GAMESERVER_WIN;
    player2.expected_outcome[0] = MSG_GAMESERVER_LOSE;
    player2.expected_outcome[1] = MSG_GAMESERVER_QUIT;
    player1.n_of_moves = 1;
    player2.n_of_moves = 2;

    tid_1 = Create(P_MED, generic_client);
    Send(tid_1, (char *)&player1, sizeof(struct game_msg), NULL, 0);
    tid_2 = Create(P_MED, generic_client);
    Send(tid_2, (char *)&player2, sizeof(struct game_msg), NULL, 0);

    Receive(&waiting_tid, NULL, 0); // waiting for processes to finish
    Reply(waiting_tid, NULL, 0);
    Receive(&waiting_tid, NULL, 0);
    Reply(waiting_tid, NULL, 0);

    wait_for_next_test();


    // Test 7: multiple clients waiting
    TEST_LOG("-------Starting Test 6: First Player 1 and 2 and later 3 and 4 play-------");
    player1.move[0] = GAME_SCISSORS;
    player1.move[1] = GAME_SCISSORS;
    player2.move[0] = GAME_PAPER;
    player2.move[1] = GAME_ROCK;
    player1.expected_outcome[0] = MSG_GAMESERVER_WIN;
    player1.expected_outcome[1] = MSG_GAMESERVER_LOSE;
    player2.expected_outcome[0] = MSG_GAMESERVER_LOSE;
    player2.expected_outcome[1] = MSG_GAMESERVER_WIN;
    player1.n_of_moves = 2;
    player2.n_of_moves = 2;

    // same for 3 and 4
    player3.move[0] = GAME_SCISSORS;
    player3.move[1] = GAME_SCISSORS;
    player4.move[0] = GAME_PAPER;
    player4.move[1] = GAME_ROCK;
    player3.expected_outcome[0] = MSG_GAMESERVER_WIN;
    player3.expected_outcome[1] = MSG_GAMESERVER_LOSE;
    player4.expected_outcome[0] = MSG_GAMESERVER_LOSE;
    player4.expected_outcome[1] = MSG_GAMESERVER_WIN;
    player3.n_of_moves = 2;
    player4.n_of_moves = 2;


    // creating all at once
    tid_1 = Create(P_MED, generic_client);
    Send(tid_1, (char *)&player1, sizeof(struct game_msg), NULL, 0);
    tid_2 = Create(P_MED, generic_client);
    Send(tid_2, (char *)&player2, sizeof(struct game_msg), NULL, 0);
    tid_3 = Create(P_MED, generic_client);
    Send(tid_3, (char *)&player3, sizeof(struct game_msg), NULL, 0);
    tid_4 = Create(P_MED, generic_client);
    Send(tid_4, (char *)&player4, sizeof(struct game_msg), NULL, 0);


    Receive(&waiting_tid, NULL, 0); // waiting for processes to finish
    Reply(waiting_tid, NULL, 0);
    Receive(&waiting_tid, NULL, 0);
    Reply(waiting_tid, NULL, 0);
    Receive(&waiting_tid, NULL, 0); // waiting for processes to finish
    Reply(waiting_tid, NULL, 0);
    Receive(&waiting_tid, NULL, 0);
    Reply(waiting_tid, NULL, 0);


    TEST_LOG("----This was the last test!!! - Press any key to finish----");
    uart_getc(CONSOLE);
}
