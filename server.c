/*** server.c ***/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include "server.h"
// #include "game_logic.h"


int main (int argc, char **argv) 
{
    int sockfd, portno, optval; /* port to listen on */
	char *hostaddrp; /* dotted decimal host addr string */
	struct sockaddr_in serveraddr; /* server's addr */
	struct hostent *hostp; /* client host info */
	struct timeval *t = NULL;

	if (argc != 2) {
		fprintf(stderr, "usage: %s <port>\n", argv[0]);
		exit(1);
	}
	portno = atoi(argv[1]);

	/* socket: create the parent socket */
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) 
		fprintf(stderr, "ERROR opening socket");
	// fprintf(stderr, "sockfd %d\n", sockfd);
	optval = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, 
			  (const void *)&optval , sizeof(int));

	/* build the server's Internet address */
	bzero((char *) &serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons((unsigned short)portno);

	/* bind: associate the parent socket with a port */
	if (bind(sockfd, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0) 
		fprintf(stderr, "ERROR on binding");

    Game game = malloc(sizeof(*game));
    assert (game != NULL);

    initialize_game(game, sockfd);

    while(1) {
        game->read_fd_set = game->active_fd_set;
        switch(game->server_state) 
        {
            // handles: register client, exit client, move instruction
            case RECEIVE: {
                bool timed_out = receive_data(game);
                if (timed_out) {
                    game->server_state = UPDATE;
                    // reset the timeout for the next tick
                }
                break;
            }
            
            case UPDATE: {

                break;
            }
            
            case SEND: {
                break;
            }
        }
    }
}

// todo review the valgrind initialization error for:
// struct sockaddr_in {
//     sa_family_t    sin_family; /* address family: AF_INET */
//     in_port_t      sin_port;   /* port in network byte order */
//     struct in_addr sin_addr;   /* internet address */
// };

// will change states using select()
// only change to 'update' state when the timeout elapses
// else, stay in the state to get more input (move instr) from clients (same or diff)
bool receive_data(Game game)
{
    char buf[MAX_CLIENT_MSG];
	bzero(buf, MAX_CLIENT_MSG);
    
    struct sockaddr_in *clientaddr = malloc(sizeof(*clientaddr));
    assert (clientaddr != NULL);
    int *clientlen = malloc(sizeof(*clientlen));
    assert(clientlen != NULL);

    /* Block until input arrives on an active socket or if timeout expires. */
	if (select (FD_SETSIZE, game->read_fd_set, NULL, NULL, NULL) < 0) {
		fprintf(stderr, "error in Select\n");
	}

    // if (timedout in select call)
    // return true; 

	int n = recvfrom(game->sockfd, buf, MAX_CLIENT_MSG, 0, (struct sockaddr *) clientaddr, clientlen);
	if (n < 0)
		fprintf(stderr, "ERROR in recvfrom\n");

    switch (buf[0]) {
        case REGISTER: {
            fprintf(stderr, "register player\n");
            register_player(game, buf + PLAYER_NAME_INDEX, clientaddr,
                           clientlen);
            start_game(game);
            print_players(game);
            break;
        }

        case EXIT: {
            fprintf(stderr, "exit\n");
            remove_player(game, buf + PLAYER_NAME_INDEX);
            print_players(game);
            break;
        }

        case MOVE: {
            fprintf(stderr, "move\n");
            register_move(game, buf);
            break;
        }
    }
}

bool start_game(Game game) 
{
    
}


// adds the next move to execute to a player's struct
void register_move(Game game, char *buf)
{
    Player found_p = find_player(game, buf + PLAYER_NAME_INDEX);
    
    if (found_p != NULL) {
        Direction d = (Direction)buf + MOVE_INSTR_INDEX;
        
        if (d == UP || d == RIGHT || d == DOWN || d == LEFT) {
            int move_sequence = atoi(buf + MOVE_SEQUENCE_INDEX);
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

    strncpy(new_p->name, name, PLAYER_NAME_LEN);

    new_p->phys.x = START_X;
    new_p->phys.y = START_Y;
    new_p->phys.d = START_D;
    
    new_p->player_state = SPECTATING;

    new_p->addr_len    = clientlen;
    new_p->player_addr = clientaddr;

    new_p->next = NULL;

    return new_p;
}

// remove a player based on the name
void remove_player(Game game, char *name)
{
    fprintf(stderr, "removing player %s\n", name );
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
    game->num_registered_players--;
}

// todo: initialize the timeout in an intelligent way
// todo: might need better logic for initializing players
void initialize_game(Game game, int sockfd)
{
    // map
    game->map = NULL;

    // players
    game->players_head = NULL;
    game->players_tail = NULL;
    clear_all_players(game);

    // states
    game->game_state   = WAITING;
    game->server_state = RECEIVE;

    // sockets
    game->active_fd_set = malloc(sizeof(*game->active_fd_set));
    assert(game->active_fd_set != NULL);
    game->sockfd = sockfd;
    FD_ZERO(game->active_fd_set);    // Initialize set of active sockets.
    FD_SET(game->sockfd, game->active_fd_set); 	// add main socket to set
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



















