/*** server.h ***/


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
    PlayerState player_state;
    PlayerPhysics phys;
} *Player;

typedef struct Game {
    int  num_players;
    char map[32][32];  //TODO: decide on definite map size
    enum GameState game_state;
    Player head;
    Player tail;
} *Game;



