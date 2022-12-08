/*** game_logic.c ***/
#include <stdlib.h>
#include <stdio.h> 
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <ncurses.h>
#include <assert.h>

#include "server.h"
#include "game_logic.h"
#include "player.h"


void get_curr_coords(Game g, PlayerPhysics *old_coords);
void get_new_coords(Game g, PlayerPhysics *old_coords, PlayerPhysics *new_coords);

// return 1 if there's a wall, else 0
int check_if_wall(Game g, int x, int y);

void check_player_collisions(Game g, PlayerPhysics *old_coords, PlayerPhysics *new_coords);
void calculate_scores(Game g);

// for all three collision helpers below,
// return 0 means no collision
// return 1 means some collision
int check_direct_overlap(PlayerPhysics *coords, int playerA, int playerB);
//int check_partial_overlap();
int check_passthrough(PlayerPhysics *old_coords, PlayerPhysics *new_coords, int playerA, int playerB);

Player get_nth_active_player(Game g, int n);

void load_map(char *file_name, Game g) {
    FILE *fptr = fopen(file_name, "rb");
    if (fptr == NULL) {
        fprintf(stderr, "Map does not exist at specified location\n");
        exit(1);
    }

    g->map = malloc(MWIDTH * MHEIGHT);
    int n = fread(g->map, 1, MWIDTH * MHEIGHT, fptr);
    if (n != (MWIDTH * MHEIGHT)) {
        fprintf(stderr, "Map is not of right size, or was read improperly\n");
        exit(1); 
    }

    fclose(fptr);
}


// place malloc'd array of data for message 3
// into the "char *update_to_send" field
// will be freed by server code
void update(Game g) {
    int msg_size = 4 + 1 + (g->num_active_players * 22);
    char *msg = malloc(msg_size);
    assert(msg != NULL);
    bzero(msg, msg_size);


    // this is arbitrarily set for now; TODO: change seq_no to be good
    int seq_no = 5;
    seq_no = htonl(seq_no);
    memcpy(msg, &seq_no, 4);

    msg[4] = g->num_active_players; 
    int buf_pos = 5;


    // get what the coordinates would be assuming no player collisions
    PlayerPhysics old_coords[g->num_active_players];
    PlayerPhysics new_coords[g->num_active_players]; 
 
    get_curr_coords(g, old_coords);
    // update coords and take walls into consideration
    get_new_coords(g, old_coords, new_coords); 

    check_player_collisions(g, old_coords, new_coords);        

    int player_number = 0;
    Player curr_player = g->players_head;
    while (curr_player != NULL) {
        if (curr_player->player_state != PLAYING) {
            curr_player = curr_player->next;
            continue;     
        }
        
        curr_player->phys.x = new_coords[player_number].x;
        curr_player->phys.y = new_coords[player_number].y;
        curr_player->phys.d = new_coords[player_number].d;

        /*fprintf(stderr, "player %s", curr_player->name);
        fprintf(stderr, " is moving %d", curr_player->phys.d);
        fprintf(stderr, " updating loc to %d %d\n", curr_player->phys.x, curr_player->phys.y);*/

        memcpy(msg + buf_pos, curr_player->name, PLAYER_NAME_LEN);
        buf_pos = buf_pos + PLAYER_NAME_LEN;
        msg[buf_pos] = curr_player->phys.x;
        buf_pos = buf_pos + 1;
        msg[buf_pos] = curr_player->phys.y;
        buf_pos = buf_pos + 1;

        player_number = player_number + 1;
        curr_player = curr_player->next;
    }

    // int i;
    // for (i = 0; i < msg_size; i++) {
    //     fprintf(stderr, "%c", msg[i]);
    // }
    // fprintf(stderr, "\n");
    g->update_to_send = msg;
}


void get_curr_coords(Game g, PlayerPhysics *old_coords) {
    Player p = g->players_head;
    int n = 0;
    while (p != NULL) {
        if (p->player_state != PLAYING) {
            p = p->next;
            continue;
        }
        assert(p->phys.x > 0 && p->phys.x < MWIDTH && p->phys.y > 0 && p->phys.y < MHEIGHT);
        assert(p->phys.d >= 0 && p->phys.d < 4);
        //fprintf(stderr, "player location: %d %d %d\n", p->phys.x, p->phys.y, p->phys.d);
        old_coords[n].x = p->phys.x; 
        old_coords[n].y = p->phys.y;
        old_coords[n].d = p->phys.d;

        n = n + 1;
        p = p->next;
    }
}

void get_new_coords(Game g, PlayerPhysics *old_coords, PlayerPhysics *new_coords) {
    int i, x, y;
    //fprintf(stderr, "active players: %d\n", g->num_active_players);
    //fprintf(stderr, "head: %s\n", g->active_p_head->name);
    //fprintf(stderr, "active head: %s\n", g->players_head->name);
    //fprintf(stderr, "active head neighbor: %s\n", g->active_p_head->next->name);
    //fprintf(stderr, "tail: %s\n", g->players_tail->name);
    for (i = 0; i < g->num_active_players; i++) {
        // have to split up cases since up/down is only a move by one 
        if (old_coords[i].d == UP || old_coords[i].d == DOWN) {
            if (old_coords[i].d == UP) {
               x = old_coords[i].x;
               y = old_coords[i].y - 1;  
            } else {
               x = old_coords[i].x;
               y = old_coords[i].y + 1;  
            }

            // double wall check to allow for bigger player characters
            if (check_if_wall(g, x, y) || check_if_wall(g, x + 1, y)) {
                new_coords[i].x = old_coords[i].x;
                new_coords[i].y = old_coords[i].y;     
                new_coords[i].d = old_coords[i].d;
            } else {
                new_coords[i].x = x;
                new_coords[i].y = y; 
                new_coords[i].d = old_coords[i].d;
            }
        } 
        else { // this is the case where player is moving left or right
            if (old_coords[i].d == RIGHT) {
               x = old_coords[i].x + 1;
               y = old_coords[i].y;  
            } else {
               x = old_coords[i].x - 1;
               y = old_coords[i].y;  
            } 
            // double wall check to allow for bigger player characters
            if (check_if_wall(g, x, y) || check_if_wall(g, x + 1, y)) {
                new_coords[i].x = old_coords[i].x;
                new_coords[i].y = old_coords[i].y;     
                new_coords[i].d = old_coords[i].d;
            } else {
                new_coords[i].x = x;
                new_coords[i].y = y;     
                new_coords[i].d = old_coords[i].d;

                // see if they can move a second space in same direction
                if (old_coords[i].d == RIGHT) {
                    x = x + 1;
                } else {
                    x = x - 1;
                }

                if (!(check_if_wall(g, x, y) || check_if_wall(g, x + 1, y))) {
                    new_coords[i].x = x;
                }
            }
        }
    }
}

int check_if_wall(Game g, int x, int y) {
    int n = (MWIDTH * y) + x;
    //fprintf(stderr, "max wall size is %d\n", (MWIDTH * MHEIGHT));
    //fprintf(stderr, "wall at index %d * %d = %d\n", x, y, n);
    
    assert(g != NULL);
    assert(g->map != NULL);
    //fprintf(stderr, "%c\n", (g->map)[n]);
    if ((g->map)[n] == '1') {
         return 1; 
    }
    return 0;
}

void check_player_collisions(Game g, PlayerPhysics *old_coords, PlayerPhysics *new_coords) {
    assert(old_coords != NULL && new_coords != NULL);

    Player p = g->players_head;
    int n = 0;
    while (p != NULL) {
        if (p->player_state != PLAYING) {
            p = p->next;
            continue;
        }
        assert(p->phys.x > 0 && p->phys.x < MWIDTH && p->phys.y > 0 && p->phys.y < MHEIGHT);

        // check against all other player positions to see if there's overlap
        int i;
        for (i = n + 1; i < g->num_active_players; i++) {
            // direct overlap (same new coord positions)
            if (check_direct_overlap(new_coords, n, i)) {
                // fprintf(stderr, "direct overlap detected\n");
                
                // have to be careful to cover case when one player is 'still' i.e.
                // running into a wall, and if they are, then the other player has
                // to be the one who gets reverted to their old position
                if ((new_coords[n].x == old_coords[n].x) && (new_coords[n].y == old_coords[n].y)) {
                    new_coords[i].x = old_coords[i].x;
                    new_coords[i].y = old_coords[i].y;
                } else {
                    new_coords[n].x = old_coords[n].x;
                    new_coords[n].y = old_coords[n].y;
                }


                // give the minotaur a score increase if they were part of collision
                // and mark the player as 'collided_with'
                if (n == 0 || i == 0) {
                    Player mino = g->active_p_head;
                    assert(mino != NULL);
                    mino->score = mino->score + 1;

                    if (n == 0) {
                        Player human = get_nth_active_player(g, i);
                        assert(human != NULL);
                        human->collided_with = true;
                        // reset the player location  
                        new_coords[i].x = 6;
                        new_coords[i].y = 3;
                    } else {
                        Player human = get_nth_active_player(g, n);
                        assert(human != NULL);
                        human->collided_with = true;
                        // reset the player location 
                        human->phys.x = 6;
                        human->phys.y = 3;
                    }
                }


                // restart the while loop to verify new collision correction
                n = 0;
                p = g->players_head;
                continue;
            }
            
            /*** scrapped the idea of partial overlap for now
            // partial overlap (clients have width=2, so partial overlap is possible)
            if (check_partial_overlap(new_coords, n, i)) {
                fprintf(stderr, "partial overlap detected\n");
                    
            }
            ***/
            
            // pass-through overlap (passed through each other)
            if (check_passthrough(old_coords, new_coords, n, i)) {
                // fprintf(stderr, "passthrough detected\n");
                new_coords[i].x = old_coords[i].x;
                new_coords[i].y = old_coords[i].y;
                new_coords[n].x = old_coords[n].x;
                new_coords[n].y = old_coords[n].y;
                

                // give the minotaur a score increase if they were part of collision
                if (n == 0 || i == 0) {
                    Player mino = g->active_p_head;
                    assert(mino != NULL);
                    mino->score = mino->score + 1;

                    // determine which client is the human and update their object
                    if (n == 0) {
                        Player human = get_nth_active_player(g, i);
                        assert(human != NULL);
                        human->collided_with = true;
                        // reset the player location
                        new_coords[i].x = 6;
                        new_coords[i].y = 3;
                    } else {
                        Player human = get_nth_active_player(g, n);
                        assert(human != NULL);
                        human->collided_with = true;
                        // reset the player location 
                        new_coords[n].x = 6;
                        new_coords[n].y = 3;
                    }
                }


                // restart the while loop to verify new collision correction
                n = 0;
                p = g->players_head;
                continue;
            }

        }

        
        n = n + 1;
        p = p->next;
    }           
}

int check_passthrough(PlayerPhysics *old_coords, PlayerPhysics *new_coords, int playerA, int playerB) {
    int a = playerA;
    int b = playerB;

    // first check to make sure both players are moving in opposite directions, this is
    // the only case when a passthrough can happen
    // use modmath to get the opposite directions (up = 0, right = 1, down = 2, left = 3)
    if (((new_coords[a].d + 2) % 4) == new_coords[b].d) {

        // now that we know they're moving in opposite directions, we can check whether
        // they passed through each other
        if ((old_coords[a].x == new_coords[b].x) && (old_coords[a].y == new_coords[b].y)) {
            return 1;
        }
    }
    
    return 0;
}


int check_direct_overlap(PlayerPhysics *coords, int playerA, int playerB) {
    int a = playerA;
    int b = playerB;

    assert(coords[a].x > 0 && coords[a].x < MWIDTH && coords[a].y > 0 && coords[a].y < MHEIGHT);
    assert(coords[b].x > 0 && coords[b].x < MWIDTH && coords[b].y > 0 && coords[b].y < MHEIGHT);

    if ((coords[a].x == coords[b].x) && (coords[a].y == coords[b].y)) {
        return 1;
    }

    return 0;
}


/**** partial overlap seems to be very complex, our solution is to make all player
 **** movement L and R be in even-numbered squares so that the width-2 aspect of
 **** player icons never leads to partial overlaps 
 ****
int check_partial_overlap(PlayerPhysics *coords, int playerA, int playerB) {
    int a = playerA;
    int b = playerB;
    
    assert(coords[a].x > 0 && coords[a].x < MWIDTH && coords[a].y > 0 && coords[a].y < MHEIGHT);
    assert(coords[b].x > 0 && coords[b].x < MWIDTH && coords[b].y > 0 && coords[b].y < MHEIGHT);

    if (((coords[a].x + 1) == coords[b].x) && (coords[a].y == coords[b].y)) {
        return 1;
    }

    if ((coords[a].x  == (coords[b].x + 1)) && (coords[a].y == coords[b].y)) {
        return 1;
    }
}
****/


void calculate_scores(Game g) {
    Player p = g->players_head;
    int n = 0;
    while (p != NULL) {
        if (p->player_state != PLAYING) {
            p = p->next;
            continue;
        }

        if ((n != 0) && (!p->collided_with)) {
            p->score = p->score + 1;
        }

        p = p->next;
        n = n + 1;
    }
}

Player get_nth_active_player(Game g, int n) {
    // 0-indexed
    assert(n < g->num_active_players);

    Player p = g->players_head;
    int i = 0;
    while (p != NULL) {
        if (p->player_state != PLAYING) {
            p = p->next;
            continue;
        }
        if (i == n) {
            return p;
        }
        p = p->next;
        i = i + 1;
    }
}
