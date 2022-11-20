/*** server.h ***/
#ifndef SERVER_HEADER
#define SERVER_HEADER

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <time.h>
#include <inttypes.h>


#include "player.h"
#include "game_logic.h"

#define ROWS 32
#define COLUMNS 32
#define MAX_CLIENT_MSG 26 // the max number of bytes the server can receive 
                         // from a client
#define MAX_ACTIVE_PLAYERS 2
#define MOVE_INSTR_INDEX 25
#define MOVE_SEQUENCE_INDEX 21
#define PLAYER_NAME_INDEX 1
#define MAX_DATA_LEN 512

typedef struct Player *Player;
#define PLAYER_NAME_LEN 20
#define BILLION 1000000000L

int ROUND_TIME; //20 // todo change: n seconds

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
    LAUNCH,
    IN_PLAY,
    END_OF_GAME,
} GameState;

typedef struct Game {
    int         sockfd;
    GameState   game_state;
    ServerState server_state;
    int         num_active_players;     // those currently playing
    int         num_registered_players; // those playing and spectating
    Player      players_head;
    Player      players_tail; // p1 -> p2 -> p3 -> p4
    Player      active_p_head; // first active player, p3
    fd_set      *active_fd_set, *read_fd_set;
    char        *map;            // just the walls, doesn't change
    char        *update_to_send; // for current map to send, sam to free it
                                 // to memcpy into the message data field
    int         round;
    struct timespec *start_time;
    struct timeval *timeout;
} *Game;

// defines a message the server sends clients over UDP
typedef struct __attribute__((__packed__)) Message {
    char type;
    char id[PLAYER_NAME_LEN];
    char data[MAX_DATA_LEN];
} *Message;

// yet another linked list of frames returned by game logic module
// frames need to be assigned a sequence number
// typedef struct Frame {
//     int sequence_num;
//     int num_players;
//     char pla
// } *Frame;

void start_game(Game game);
void initialize_game(Game game, int sockfd);
bool receive_data(Game game);
void print_game_state(Game game);
void reset_timeout(Game game);
void set_start_time(Game game);
int64_t get_current_time();
int64_t time_in_billion(Game game);
bool is_round_over(Game game);

// send messages to clients:
void send_to_all(Game game, char *msg, int size);
void send_map(Game game);
void send_start_notification(Game game);
void send_end_game_notifcation(Game game);
void update_players(Game game);

#endif
