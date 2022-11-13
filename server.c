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
	// fprintf(stderr, "socfd %d\n", sockfd);
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
            case RECEIVE: 
            {
                // will change states using select()
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

// todo: initialize the timeout in an intelligent way
void initialize_game(Game game, int sockfd)
{
    // map
    game->map = malloc(sizeof(char) * ROWS * COLUMNS);
    assert(game->map != NULL);

    // players
    game->player_list = NULL;
    clear_all_players(game);

    // states
    game->game_state = WAITING;
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
    if (game->player_list != NULL) {
        Player curr = game->player_list->head;
        while (curr != NULL) {
            free(curr);
            curr = curr->next;
        }
    }
}





















