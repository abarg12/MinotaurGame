/*** server.h ***/
#ifndef SERVER_HEADER
#define SERVER_HEADER

#include <assert.h>
#include <stdbool.h>

#include "game_logic.h"

#define ROWS 32
#define COLUMNS 32
#define MAX_CLIENT_MSG 26 // the max number of bytes the server can receive 
                         // from a client
#define START_X 10
#define START_Y 10
#define START_D 1 // RIGHT
#define PLAYER_NAME_LEN 20

typedef enum MessageType {
    REGISTER = 0,
    EXIT     = 1,
    MOVE     = 2,
} MessageType;

typedef enum ServerState {
    RECEIVE,
    UPDATE,
    SEND, 
} ServerState;

typedef enum GameState {
    WAITING,
    IN_PLAY,
    END_OF_GAME,
} GameState;

typedef enum PlayerState {
    SPECTATING,
    PLAYING,
} PlayerState;

// list of players
typedef struct Player {
    char name[PLAYER_NAME_LEN];
    PlayerPhysics phys;
    PlayerState player_state;
    int *addr_len;
    struct sockaddr_in *player_addr;
    struct Player *next;
    Direction d; // update when get a move instruction
    int    last_move; // prevents server from executing an out of order move
} *Player;

// typedef struct Frame {
//     int sequence_num;
//     int num_players;
//     char pla
// } *Frame;

typedef struct Game {
    int         sockfd;
    GameState   game_state;
    ServerState server_state;
    int         num_active_players;     // those currently playing
    int         num_registered_players; // those playing and spectating
    Player      players_head;
    Player      players_tail;
    fd_set      *active_fd_set, *read_fd_set;
    struct timeval **timeout_p;
} *Game;

// yet another linked list of frames returned by game logic module
// frames need to be assigned a sequence number

void initialize_game(Game game, int sockfd);
void clear_all_players(Game game);
// void clear_all_moves(Game game);
bool receive_data(Game game);
Player create_new_player(Game game, char *name, struct sockaddr_in *clientaddr, 
                       int *clientlen);
void add_player_to_list(Game game, Player p);
void register_player(Game game, char *name, struct sockaddr_in *clientaddr, int *clientlen);
void register_move(Game game, char *buf);
void print_players(Game game);
void remove_player(Game game, char *name);



#endif
