#include "player.h"

// stores the next move to execute in a player's struct
// only stores a move whose sequence number is greater than the one previously
// executed (in case of out of order UDP packet)
void register_move(Game game, char *buf)
{
    Player found_p = find_player(game, buf + PLAYER_NAME_INDEX);
    
    if (found_p != NULL) {
        fprintf(stderr, "found player %s\n", found_p->name);
        Direction d = (Direction)buf[MOVE_INSTR_INDEX];
        print_move_direction(d);
        
        if (d == UP || d == RIGHT || d == DOWN || d == LEFT) {

            int s;
            memcpy(&s, buf + MOVE_SEQUENCE_INDEX, 4);

            uint32_t move_sequence = ntohl(s);
            fprintf(stderr, "move sequence %d\n", move_sequence);
            if (move_sequence > found_p->last_move) {
                found_p->phys.d = d;
                found_p->last_move = move_sequence;
            }
        }

    } else {
        fprintf(stderr, "Player not registerd!\n");
    }
}

// find and return a player by name in the list of registered players.
// if player not found, return null.
Player find_player(Game game, char *name)
{
    Player curr = game->players_head;
    while (curr != NULL) {
        if (strncmp(curr->name, name, PLAYER_NAME_LEN) == 0) {
            return curr;
        }
        curr = curr->next;
    }
    return NULL;
}

// registers a player in the game
void register_player(Game game, char *name, struct sockaddr_in *clientaddr, 
                     int *clientlen)
{   
   Player new_p = create_new_player(game, name, clientaddr, clientlen);
   add_player_to_list(game, new_p);
}

// adds a given player to list of players
void add_player_to_list(Game game, Player p)
{
    // list is empty, add to front
    if (game->players_head == NULL) {
        game->players_head = p;
        game->players_tail = p;
        game->active_p_head = p;
    
    // add to end
    } else {
        game->players_tail->next = p;
        game->players_tail = p;
    }
    game->num_registered_players++;
}

Player create_new_player(Game game, char *name, struct sockaddr_in *clientaddr, 
                         int *clientlen)
{
    Player new_p = malloc(sizeof(*new_p));
    assert(new_p != NULL);

    bzero(new_p->name, PLAYER_NAME_LEN);
    strncpy(new_p->name, name, PLAYER_NAME_LEN);

    new_p->score = 0;

    new_p->phys.x = START_X;
    new_p->phys.y = START_Y;
    new_p->phys.d = START_D;
    
    new_p->player_state = SPECTATING;

    new_p->addr_len   = *clientlen;
    new_p->player_addr = clientaddr;

    new_p->next = NULL;

    return new_p;
}

// remove a player based on the name
void remove_player(Game game, char *name)
{
    Player found_p = find_player(game, name);
    if (found_p != NULL) {
        fprintf(stderr, "removing player %s\n", name );
        game->num_registered_players--;

        Player curr = game->players_head;
        Player prev = curr;
        while (curr != NULL) {
            if (strcmp(curr->name, name) == 0) {
                if (curr == game->players_head) {
                    game->players_head = curr->next;
                } else if (curr == game->players_tail) {
                    game->players_tail = prev;
                } else {
                    prev->next = curr->next;
                }
                free(curr);
                break;
            }
            prev = curr;
            curr = curr->next;
        }
    }
}

void print_players(Game game)
{
    if (game->players_head == NULL)
        fprintf(stderr, "no players \n");
    else {
        fprintf(stderr, "players:\n");

        Player curr = game->players_head;
        while (curr != NULL) {
            fprintf(stderr, "%s\n", curr->name);
            curr = curr->next;
        }
    }
}

// todo: update counts of registered/active players
void clear_all_players(Game game)
{   
    Player curr = game->players_head;
    while (curr != NULL) {
        free(curr);
        curr = curr->next;
    }
}

void print_move_direction(Direction d)
{
    char direction[15] = {0};

    switch(d) {
        case UP:
            strcpy(direction, "UP");
            break;
        case RIGHT:
            strcpy(direction, "RIGHT");
            break;
        case DOWN:
            strcpy(direction, "DOWN");
            break;
        case LEFT:
            strcpy(direction, "LEFT");
            break;
    }
    fprintf(stderr, "Direction: %s\n", direction);
}