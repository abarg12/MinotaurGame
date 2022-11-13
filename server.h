/*** server.h ***/
#ifndef SERVER_HEADER
#define SERVER_HEADER

#include <assert.h>

#define ROWS 32
#define COLUMNS 32

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

typedef enum Direction {
    UP,
    RIGHT,
    DOWN,
    LEFT, 
} Direction; 

typedef struct PlayerPhysics {
    int x;
    int y;
    Direction d; 
} PlayerPhysics;

typedef struct Player {
    PlayerPhysics phys;
    PlayerState player_state;
    int *addr_len;
    struct sockaddr_in *player_addr;
    struct Player *next;
} *Player;

typedef struct PlayerList {
    int    num_players;
    Player head;
    Player tail;
}*PlayerList;

typedef struct Game {
    char        *map;
    int         sockfd;
    GameState   game_state;
    ServerState server_state;
    PlayerList  player_list;
    fd_set      *active_fd_set, *read_fd_set;
    struct timeval **timeout_p;
} *Game;


void initialize_game(Game game, int sockfd);
void clear_all_players(Game game);


#endif