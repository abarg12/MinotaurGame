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


    int player_number = 0;
    Player curr_player = g->players_head;
    while (curr_player != NULL) {
        if (curr_player->player_state != PLAYING) {
            curr_player = curr_player->next;
            continue;     
        }
        
            //TODO: check for collisions between player characters

        curr_player->phys.x = new_coords[player_number].x;
        curr_player->phys.y = new_coords[player_number].y;
        curr_player->phys.d = new_coords[player_number].d;

        memcpy(msg + buf_pos, curr_player->name, PLAYER_NAME_LEN);
        buf_pos = buf_pos + PLAYER_NAME_LEN;
        msg[buf_pos] = curr_player->phys.x;
        buf_pos = buf_pos + 1;
        msg[buf_pos] = curr_player->phys.y;
        buf_pos = buf_pos + 1;

        player_number = player_number + 1;
        curr_player = curr_player->next;
    }

    int i;
    for (i = 0; i < msg_size; i++) {
        fprintf(stderr, "%c", msg[i]);
    }
    fprintf(stderr, "\n");
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
        fprintf(stderr, "player location: %d %d %d\n", p->phys.x, p->phys.y, p->phys.d);
        old_coords[n].x = p->phys.x; 
        old_coords[n].y = p->phys.y;
        old_coords[n].d = p->phys.d;

        n = n + 1;
        p = p->next;
    }
}

void get_new_coords(Game g, PlayerPhysics *old_coords, PlayerPhysics *new_coords) {
    int i, x, y;
    fprintf(stderr, "active players: %d\n", g->num_active_players);
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
    fprintf(stderr, "checking for wall on coordinates: %d   ", x);
    fprintf(stderr, "%d\n", y);
    if ((g->map)[n] == '1') {
         return true; 
    }
    return false;
}

