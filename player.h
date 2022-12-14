#ifndef PLAYER_HEADER
#define PLAYER_HEADER

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#include "game_logic.h"
#include "server.h"

#define MOVE_INSTR_INDEX 25
#define MOVE_SEQUENCE_INDEX 21
#define PLAYER_NAME_INDEX 1
#define PLAYER_NAME_LEN 20
#define MAX_UNACKED 3
#define SUCCESS '1'
#define FAILURE '0'

typedef enum PlayerState {
    SPECTATING,
    PLAYING,
} PlayerState;

// list of players
typedef struct Player {
    char name[PLAYER_NAME_LEN];
    PlayerPhysics phys;
    PlayerState player_state;
    int addr_len;
    struct sockaddr_in *player_addr;
    int last_move; // prevents server from executing an out of order move
    int score;
    bool collided_with;
    int unacked; // tracks how many PINGS have not been ACK'd by player
    struct Player *next;
} *Player;

Player create_new_player(Game game, char *name, struct sockaddr_in *clientaddr, 
                       int *clientlen);
void add_player_to_list(Game game, Player p);
void register_player(Game game, char *name, struct sockaddr_in *clientaddr, 
                     int *clientlen);
Player find_idle_players(Game game);
void remove_idle_players(Game game);
Player find_player(Game game, char *name);
void register_move(Game game, char *buf);
void print_players(Game game);
void remove_player(Game game, char *name);
void clear_all_players(Game game);
void print_move_direction(Direction d);
void incr_ping_tracker(Game game);
void reset_unacked(Game game, char *name);
bool check_unique(Game game, char *name);

#endif