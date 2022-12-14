#include "player.h"

// stores the next move to execute in a player's struct
// only stores a move whose sequence number is greater than the one previously
// executed (in case of out of order UDP packet)
void register_move(Game game, char *buf)
{
    Player found_p = find_player(game, buf + PLAYER_NAME_INDEX);
    
    if (found_p != NULL) {
        // fprintf(stderr, "found player %s\n", found_p->name);
        Direction d = (Direction)buf[MOVE_INSTR_INDEX];
        // print_move_direction(d);
        
        if (d == UP || d == RIGHT || d == DOWN || d == LEFT) {

            int s;
            memcpy(&s, buf + MOVE_SEQUENCE_INDEX, 4);

            int move_sequence = ntohl(s);
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

// registers a player in the game and sends a registration ACK
void register_player(Game game, char *name, struct sockaddr_in *clientaddr, 
                     int *clientlen)
{   
    Player new_p  = create_new_player(game, name, clientaddr, clientlen);
    bool unique_p = check_unique(game, name);

    if (unique_p) {
        add_player_to_list(game, new_p);
        send_player_registration_ack(game, new_p, SUCCESS);
    } else {
        send_player_registration_ack(game, new_p, FAILURE);
        free(new_p);
    } 
}

// returns true if the given name doesn't yet exist in the list of registered players, false otherwise.
bool check_unique(Game game, char *name)
{
    Player curr = game->players_head;
    while (curr != NULL) {
        if (strcmp(curr->name, name) == 0) {
            return false;
        }
        curr = curr->next;
    }
    return true;
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
    new_p->collided_with = false;
    new_p->last_move = -1;

    new_p->phys.x = START_X;
    new_p->phys.y = START_Y;
    new_p->phys.d = START_D;
    
    new_p->player_state = SPECTATING;

    new_p->addr_len   = *clientlen;
    new_p->player_addr = clientaddr;

    new_p->unacked = 0;
    new_p->next = NULL;

    return new_p;
}

// for all clients, increment their unacked counter by one.
void incr_ping_tracker(Game game)
{   
    Player curr = game->players_head;
    while (curr != NULL) {
        curr->unacked++;
        curr = curr->next;
    }
}

// todo: should the ACK have an ID for it to actually BE cumulative?
// b/c receiving a super old ack shouldn't count...
// for a given client, reset the unacked counter to 0 (we are using cumulating acks for ping) when (todo: most recent) ACK is received.
void reset_unacked(Game game, char *name)
{
    Player found_p = find_player(game, name);
    if (found_p != NULL) {
        found_p->unacked = 0;
    }
}

Player find_idle_players(Game game)
{
    Player curr = game->players_head;
    while (curr != NULL) {   
        if (curr->unacked == MAX_UNACKED) {
            // fprintf(stderr, "removing idle player: %s\n", curr->name);
            return curr;
        }
        curr = curr->next;
    }
    return NULL;
}

// remove all idle players and handle case where players get disconnected
// mid-game by stopping the game.
void remove_idle_players(Game game)
{
    Player found_p = NULL;
    while ((found_p = find_idle_players(game)) != NULL) {
        remove_player(game, found_p->name);
    }

    if (game->game_state == IN_PLAY && 
        game->num_active_players < game->MAX_ACTIVE_PLAYERS) 
    {
        game->game_state = END_OF_GAME;
        game->server_state = SEND;
    }
}

// remove a player based on the name
void remove_player(Game game, char *name)
{
    Player found_p = find_player(game, name);

    if (found_p != NULL) {
        // fprintf(stderr, "removing player %s\n", name );
        
        game->num_registered_players--;

        if (found_p->player_state == PLAYING) {
            game->num_active_players--;
        }

        Player curr = game->players_head;
        Player prev = curr;
        while (curr != NULL) {
            if (strcmp(curr->name, name) == 0) {
                if (curr == game->players_head) {
                    game->players_head = curr->next;
                } else if (curr == game->players_tail) {
                    game->players_tail = prev;
                    game->players_tail->next = NULL;
                } else {
                    prev->next = curr->next;
                }

                // update the active_p_head pointer
                Player aph = game->active_p_head;
                if (curr == aph) {
                    if (aph->next != NULL) {
                        game->active_p_head = aph->next;
                    } else {
                        game->active_p_head = game->players_head;
                    }
                }

                free(curr);
                curr = NULL;
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
