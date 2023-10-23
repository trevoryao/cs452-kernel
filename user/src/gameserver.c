#include "gameserver.h"

#include "msg.h"
#include "util.h"
#include "nameserver.h"
#include "rpi.h"
#include "term-control.h"

#define SERVER_LOG(fmt, ...) uart_printf(CONSOLE, COL_MAG "[Game Server] " fmt COL_RST "\r\n" __VA_OPT__(,) __VA_ARGS__)

static const enum GAME_TURN GAME_TURN_WINNERS[] = {GAME_SCISSORS, GAME_PAPER, GAME_ROCK};

enum playerStatus {
    PLAYER_INACTIVE,
    PLAYER_WAITING_TO_MOVE,
    PLAYER_MADE_MOVE,
    PLAYER_QUIT
};

typedef struct player {
    uint16_t playerTid;
    enum GAME_TURN latest_gameTurn;
    enum playerStatus playerStatus;
} player;

typedef struct gameserver {
    struct msg_gameserver msg_received;
    struct msg_gameserver msg_reply;
    struct msg_gameserver msg_error;
    uint16_t waiting_players[MAX_QUEUE];
    player player1;
    player player2;
    int senderTid;
} gameserver;

static void gameserver_init(gameserver *gs) {
    gs->player1.playerStatus = PLAYER_INACTIVE;
    gs->player2.playerStatus = PLAYER_INACTIVE;
}

static void sendErrorMsg(gameserver *gs, int playerTid) {
    gs->msg_error.type = MSG_GAMESERVER_ERROR;
    gs->msg_error.playerTid = playerTid;
    Reply(playerTid, (char *)&gs->msg_error, sizeof(msg_gameserver));
}

static void sendResponseMsg(gameserver *gs, int playerTid, enum GAME_MSG_TYPE response) {
    gs->msg_reply.type = response;
    gs->msg_reply.playerTid = playerTid;
    Reply(playerTid, (char *)&gs->msg_reply, sizeof(msg_gameserver));
}

static void sendQuitMsg(gameserver *gs, int playerTid) {
    sendResponseMsg(gs, playerTid, MSG_GAMESERVER_QUIT);
}

static void sendRequestMsg(gameserver *gs, int playerTid) {
   sendResponseMsg(gs, playerTid, MSG_GAMESERVER_MOVE_REQUIRED);
}

static void gameMove(gameserver *gs) {
    // write the logic for the move
    if (gs->player1.latest_gameTurn == gs->player2.latest_gameTurn) {
        SERVER_LOG("TIE - nobody won");
        sendResponseMsg(gs, gs->player1.playerTid, MSG_GAMESERVER_TIE);
        sendResponseMsg(gs, gs->player2.playerTid, MSG_GAMESERVER_TIE);
    } else if (gs->player1.latest_gameTurn == GAME_TURN_WINNERS[gs->player2.latest_gameTurn]) {
        // player 1 has won the game
        SERVER_LOG("%d has won the game", gs->player1.playerTid);
        sendResponseMsg(gs, gs->player1.playerTid, MSG_GAMESERVER_WIN);
        sendResponseMsg(gs, gs->player2.playerTid, MSG_GAMESERVER_LOSE);
    } else {
        // player 2 has won the game
        SERVER_LOG("%d has won the game", gs->player2.playerTid);
        sendResponseMsg(gs, gs->player1.playerTid, MSG_GAMESERVER_LOSE);
        sendResponseMsg(gs, gs->player2.playerTid, MSG_GAMESERVER_WIN);
    }

    gs->player1.playerStatus = PLAYER_WAITING_TO_MOVE;
    gs->player2.playerStatus = PLAYER_WAITING_TO_MOVE;
}

void gameserver_main() {
    SERVER_LOG("Gameserver starting");
    gameserver gs;
    // init the gameserver
    gameserver_init(&gs);

    int head = 0;
    int tail = 0;
    int players_in_queue = 0;

    // Signup server at nameserver
    int response = RegisterAs(GAME_SERVER_NAME);
    SERVER_LOG("response from %d", response);
    SERVER_LOG("registered at nameserver");


    for(;;) {
        Receive(&gs.senderTid, (char *)&gs.msg_received, sizeof(msg_gameserver));
        //SERVER_LOG("received msg from tid %d", gs.senderTid);

        // read message
        switch (gs.msg_received.type) {
            case MSG_GAMESERVER_SIGNUP: {
                // add player to the waiting struct
                if (players_in_queue < MAX_QUEUE) {
                    gs.waiting_players[tail] = gs.msg_received.playerTid;
                    tail = (tail + 1) % MAX_QUEUE;
                    players_in_queue += 1;
                } else {
                    // send error if queue is full
                    sendErrorMsg(&gs, gs.msg_received.playerTid);
                }

                break;
            }
            case MSG_GAMESERVER_PLAY: {
                if (gs.player1.playerTid == gs.msg_received.playerTid) {
                    if (gs.player2.playerStatus == PLAYER_QUIT) {
                        // if the other player quit -> return quit msg & quit player as well
                        sendQuitMsg(&gs, gs.player1.playerTid);
                        gs.player1.playerStatus = PLAYER_QUIT;
                    } else {
                        gs.player1.latest_gameTurn = gs.msg_received.gameTurn;
                        gs.player1.playerStatus = PLAYER_MADE_MOVE;
                    }
                } else if (gs.player2.playerTid == gs.msg_received.playerTid) {
                    if(gs.player1.playerStatus == PLAYER_QUIT) {
                        sendQuitMsg(&gs, gs.player2.playerTid);
                        gs.player2.playerStatus = PLAYER_QUIT;
                    } else {
                        gs.player2.latest_gameTurn = gs.msg_received.gameTurn;
                        gs.player2.playerStatus = PLAYER_MADE_MOVE;
                    }
                } else {
                    // unexpected play request
                    sendErrorMsg(&gs, gs.msg_received.playerTid);
                }

                break;
            }
            case MSG_GAMESERVER_QUIT: {
                if (gs.player1.playerTid == gs.msg_received.playerTid) {
                    gs.player1.playerStatus = PLAYER_QUIT;

                    if (gs.player2.playerStatus == PLAYER_MADE_MOVE) {
                        sendQuitMsg(&gs, gs.player2.playerTid);
                        gs.player2.playerStatus = PLAYER_QUIT;
                    }
                    sendQuitMsg(&gs, gs.player1.playerTid);
                } else if (gs.player2.playerTid == gs.msg_received.playerTid) {
                    gs.player2.playerStatus = PLAYER_QUIT;

                    if(gs.player1.playerStatus == PLAYER_MADE_MOVE) {
                        sendQuitMsg(&gs, gs.player1.playerTid);
                        gs.player1.playerStatus = PLAYER_QUIT;
                    }
                    sendQuitMsg(&gs, gs.player2.playerTid);
                } else {
                    sendErrorMsg(&gs, gs.msg_received.playerTid);
                }

                break;
            }
            default: {
                // Throw exception bc of unkown send message
                sendErrorMsg(&gs, gs.msg_received.playerTid);
                break;
            }

        }

        // Check if there are two players and no ongoing game
        if (gs.player1.playerStatus == PLAYER_MADE_MOVE && gs.player2.playerStatus == PLAYER_MADE_MOVE) {
            // decide who won & respond
            gameMove(&gs);
        } else if ((gs.player1.playerStatus == PLAYER_QUIT && gs.player2.playerStatus == PLAYER_QUIT)
            || (gs.player1.playerStatus == PLAYER_INACTIVE && gs.player2.playerStatus == PLAYER_INACTIVE)) {

            // game inactive and start a new one
            if (players_in_queue >= 2) {
                // start a new game

                // put players into active players
                gs.player1.playerTid = gs.waiting_players[head];
                gs.player1.playerStatus = PLAYER_WAITING_TO_MOVE;

                head = (head + 1) % MAX_QUEUE;

                gs.player2.playerTid = gs.waiting_players[head];
                gs.player2.playerStatus = PLAYER_WAITING_TO_MOVE;

                head = (head + 1) % MAX_QUEUE;
                players_in_queue -= 2;


                // send move requests to players
                sendRequestMsg(&gs, gs.player1.playerTid);
                sendRequestMsg(&gs, gs.player2.playerTid);
                SERVER_LOG("started new game between tid %d and %d", gs.player1.playerTid, gs.player2.playerTid);
            }
        }
    }
}
