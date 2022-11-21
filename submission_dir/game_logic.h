/*** game_logic.h ***/
#ifndef GAME_LOGIC_HEADER
#define GAME_LOGIC_HEADER

// Map constants
#define MHEIGHT 28
#define MWIDTH  96

#define START_X 10
#define START_Y 10
#define START_D 1 // RIGHT

// forward declarations, i.e "will be defined later"
typedef struct Game *Game; 
typedef struct Player *Player;
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

typedef struct __attribute__((__packed__)) InstrStruct {
    char type;
    char ID[20];
    char DATA[512];
} InstrStruct;


// Load the map from a file into the map array 
void load_map(char *file_name, Game g);

// data for Message 3 from the spec gets written into the game struct 
void update(Game g);

#endif
