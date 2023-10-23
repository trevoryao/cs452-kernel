#ifndef __GAMESERVER_H__
#define __GAMESERVER_H__

#include <stdint.h>

#include "msg-type.h"

#define MAX_QUEUE 10
#define GAME_SERVER_NAME "gameserver"

enum GAME_TURN {
    GAME_PAPER,
    GAME_ROCK,
    GAME_SCISSORS,
    N_GAME_TURNS
};

enum GAME_MSG_TYPE {
    MSG_GAMESERVER_SIGNUP = N_MSG_TYPE,
    MSG_GAMESERVER_PLAY,
    MSG_GAMESERVER_QUIT,
    MSG_GAMESERVER_ERROR,
    MSG_GAMESERVER_MOVE_REQUIRED,
    MSG_GAMESERVER_WIN,
    MSG_GAMESERVER_LOSE,
    MSG_GAMESERVER_TIE,
    MSG_GAMESERVER_MAX
};

// Messages for gameserver
typedef struct msg_gameserver {
    enum GAME_MSG_TYPE type;
    uint16_t playerTid;
    enum GAME_TURN gameTurn;

} msg_gameserver;

void gameserver_main(void);

#endif
