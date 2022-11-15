/*** server.c ***/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include "server.h"


int main (int argc, char **argv) 
{
    int sockfd; /* socket */
	int portno; /* port to listen on */
	struct sockaddr_in serveraddr; /* server's addr */
	struct hostent *hostp; /* client host info */
	char *hostaddrp; /* dotted decimal host addr string */
	int optval; /* flag value for setsockopt */
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
            // process request: register client, exit client, move instruction
            case RECEIVE: 
            {
                fprintf(stderr, "receive state\n");
                receive_data(game);
                // will change states using select()
                // only exit the receive state when the timeout actually elapsed
                // else, stay in the state to get more input from another client
                // add payload
                return 0;
                break;
            }
            
            case UPDATE:
            {
                break;
            }
            
            case SEND:
            {
                break;
            }
        }
    }
}

void receive_data(Game game)
{
    char buf[MAX_CLIENT_MSG];
	bzero(buf, MAX_CLIENT_MSG);
    
    struct sockaddr_in *clientaddr = malloc(sizeof(*clientaddr));
    assert (clientaddr != NULL);

    int *clientlen = malloc(sizeof(int));
    assert(clientlen != NULL);

    /* Block until input arrives on an active socket or if timeout expires. */
    // fprintf(stderr, "waiting for data \n");
	// if (select (FD_SETSIZE, game->read_fd_set, NULL, NULL, NULL) < 0) {
	// 	fprintf(stderr, "error in Select\n");
	// }
    // fprintf(stderr, "got data \n");

	// int n = recvfrom(game->sockfd, buf, MAX_CLIENT_MSG, 0, (struct sockaddr *) 	
	// 				 clientaddr, clientlen);
	// if (n < 0)
	// 	fprintf(stderr, "ERROR in recvfrom");

    // MessageType msg_type = buf[0];
    MessageType msg_type = 0;

    switch (msg_type) {
        case REGISTER: {
            //todo ? error handling: client is already registered
            fprintf(stderr, "register player\n");
            
            // todo
            // register_player(game, buf + 1, clientaddr, clientlen);
            
            char name[PLAYER_NAME_LEN] = "Sam"; // placeholder
            register_player(game, name, clientaddr, clientlen);
            print_players(game);
            break;
        
        }
        case EXIT: {
            fprintf(stderr, "exit\n");
            // remove_player(game, buf + 1);
            char name[PLAYER_NAME_LEN] = "Sam"; // placeholder
            remove_player(game, name);
            print_players(game);
        
            break;
        }
        case MOVE: {
            fprintf(stderr, "move\n");
            // add the move instruction to a list of payload structs
            // find the player by name first
            // create new moveInstruction struct
            // need to keep tabs on which is the next instruction to execute,
        
            break;
        }
    }
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
    if (game->list_head == NULL) {
        game->list_head = p;
        game->list_tail = p;
    
    // add to end
    } else {
        game->list_tail->next = p;
        game->list_tail = p;
    }

    game->num_players++;
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
    Player curr = game->list_head;
    Player prev = curr;
    while (curr != NULL) {
        if (strcmp(curr->name, name) == 0) {
            if (curr == game->list_head) {
                game->list_head = curr->next;
            } else if (curr == game->list_tail) {
                game->list_tail = prev;
            } else {
                prev->next = curr->next;
            }
            free(curr);
            break;
        }
        prev = curr;
        curr = curr->next;
    }
    game->num_players--;
}

// todo: initialize the timeout in an intelligent way
// todo: might need better logic for initializing players
void initialize_game(Game game, int sockfd)
{
    // players
    game->list_head = NULL;
    game->list_tail = NULL;
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


void clear_all_players(Game game)
{   
    Player curr = game->list_head;
    while (curr != NULL) {
        free(curr);
        curr = curr->next;
    }
}

void print_players(Game game)
{
    if (game->list_head == NULL)
        fprintf(stderr, "no players \n");
    else {
        fprintf(stderr, "players:\n");

        Player curr = game->list_head;
        while (curr != NULL) {
            fprintf(stderr, "%s\n", curr->name);
            curr = curr->next;
        }
    }
}



















