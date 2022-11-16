/*** game_logic.h ***/
#ifndef GAME_LOGIC_HEADER
#define GAME_LOGIC_HEADER

// forward declarations, i.e "will be defined later"
typedef struct Game *Game; 
typedef struct Player *Player;
typedef struct MoveInstruction *Move;
typedef enum PlayerState PlayerState;
typedef enum ServerState ServerState;

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


// Message 3 from the spec returned
char *update(Game g);

#endif
