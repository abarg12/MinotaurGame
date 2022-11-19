/*** server.c ***/
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "server.h"
#include "game_logic.h"


int main (int argc, char **argv) 
{
    int sockfd, portno, optval; /* port to listen on */
	char *hostaddrp; /* dotted decimal host addr string */
	struct sockaddr_in serveraddr; /* server's addr */
	struct hostent *hostp; /* client host info */
	struct timeval *t = NULL;

	if (argc != 1) {
		fprintf(stderr, "usage: %s\n", argv[0]);
		exit(1);
	}
	portno = 9040;

	/* socket: create the parent socket */
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) 
		fprintf(stderr, "ERROR opening socket");
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
        FD_SET(game->sockfd, game->active_fd_set);

        switch(game->server_state) {
            case RECEIVE: {
                bool timed_out = receive_data(game);

                if (game->game_state == IN_PLAY && timed_out) {
                    game->server_state = UPDATE;
                }
                break;
            }
            
            case UPDATE: {
                fprintf(stderr, "Update state\n");
                send_map(game);
                return 0;
                // char *message_to_send = update(game);
                // game->game_state == SEND;
                break;
            }
            
            case SEND: {
                send_map(game);
                break;
            }
        }
    }
}

// sends the updated coordinates to all the registered clients
void send_map(Game game)
{
    // placeholder
    char msg[] = "This a message from the server baby!!";

    // 4  = sequence num (int)
    // 1  = num players (char)
    // 22 = player name (20 chars) + x, y coordinates (1 char)
    int msg_size = 4 + 1 + game->num_registered_players * 22;

    Player curr = game->players_head;
    while (curr != NULL) {
        if (curr->player_addr == NULL)
        {
            fprintf(stderr, "player address is NULL\n");
        }
                
        int bytes = sendto(game->sockfd, msg, sizeof(msg), 0, 
                (struct sockaddr *) curr->player_addr, curr->addr_len);

        if (bytes < 0)
            fprintf(stderr, "***ERROR in sendto: %d\n", bytes);

        curr = curr->next;
    }
}

void reset_timeout(Game game)
{
    game->timeout->tv_sec = 5;
    game->timeout->tv_usec = 0;
}

// will change states using select()
// only change to 'update' state when the timeout elapses
// else, stay in the state to get more input (move instr) from clients (same or diff)
bool receive_data(Game game)
{
    char buf[MAX_CLIENT_MSG];
	bzero(buf, MAX_CLIENT_MSG);
    
    struct sockaddr_in *clientaddr = malloc(sizeof(*clientaddr));
    assert (clientaddr != NULL);
    bzero(clientaddr, sizeof(struct sockaddr_in));

    // WHOA: must populate len before calling recvfrom, can't leave it 0
    int *clientlen = malloc(sizeof(int));
    *clientlen = sizeof(*clientaddr);
    assert(clientlen != NULL);

    /* Block until input arrives on an active socket or if timeout expires. */
	if (select (FD_SETSIZE, game->active_fd_set, NULL, NULL, game->timeout) < 
        0) {
		fprintf(stderr, "error in Select\n");
	}

    if (game->timeout->tv_usec == 0) {
        fprintf(stderr, "timed out\n");
        reset_timeout(game);
        return true;
    }

	int n = recvfrom(game->sockfd, buf, MAX_CLIENT_MSG, 0, 
                    (struct sockaddr *) clientaddr, clientlen);

	if (n < 0)
		fprintf(stderr, "ERROR in recvfrom\n");

    fprintf(stderr, "client address: %s, %d\n", inet_ntoa (clientaddr->sin_addr), ntohs (clientaddr->sin_port));
    
    switch (buf[0]) {
        case REGISTER: {
            register_player(game, buf + PLAYER_NAME_INDEX, clientaddr,
                           clientlen);
            start_game(game);
            print_players(game);
            print_game_state(game);
            break;
        }

        case EXIT: {
            remove_player(game, buf + PLAYER_NAME_INDEX);
            print_players(game);
            break;
        }

        case MOVE: {
            register_move(game, buf);
            break;
        }
    }
    return false;
}

void print_game_state(Game game)
{
    char game_state[15] = {0};
    switch(game->game_state) {
        case WAITING:
            strcpy(game_state, "WAITING");
            break;
        case IN_PLAY:
            strcpy(game_state, "IN_PLAY");
            break;
        case END_OF_GAME:
            strcpy(game_state, "END_OF_GAME");
            break;
    }
    fprintf(stderr, "Game State: %s\n", game_state);
}

// determines if the game can start based on the number of registered players
// and changes the game state to "IN_PLAY" if so
// and changes the state of two players to "PLAYING"
// todo to loop back, check if one of the active_p_head == NULL or if == tail
void start_game(Game game) 
{
    if (game->num_registered_players >= MAX_ACTIVE_PLAYERS) {
        Player curr = game->active_p_head;
        int i = 0;
        while (curr != NULL && i < MAX_ACTIVE_PLAYERS) {
             curr->player_state = PLAYING;
             curr = curr->next;
             i++;
        }
        game->game_state = IN_PLAY;
    }
}

// todo: might need better logic for initializing players
void initialize_game(Game game, int sockfd)
{
    // map
    game->map = NULL;

    // players
    game->players_head  = NULL;
    game->players_tail  = NULL;
    game->active_p_head = game->players_head;
    game->num_active_players = 0;
    game->num_registered_players = 0;
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

    // timeout
    game->timeout = malloc(sizeof(struct timeval));
    assert(game->timeout != NULL);
    game->timeout->tv_sec = 5;
    game->timeout->tv_usec = 0;
}






















