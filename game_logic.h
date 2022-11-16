/*** game_logic.h ***/
#ifndef GAME_LOGIC_HEADER
#define GAME_LOGIC_HEADER

// Map constants
#define MHEIGHT 28
#define MWIDTH  96

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

// Load the map from a file into the map array 
void load_map(char *file_name, char *map);

// Message 3 from the spec returned
char *update(Game g, char *map);

#endif
