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

	if (argc != 2) {
		fprintf(stderr, "usage: %s <round time>\n", argv[0]);
		exit(1);
	}
	portno = 9040;
    ROUND_TIME = atoi(argv[1]);

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

	if (bind(sockfd, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0) 
		fprintf(stderr, "ERROR on binding");

    Game game = malloc(sizeof(*game));
    assert (game != NULL);

    initialize_game(game, sockfd);
    int i= 0;
    while(i < 40) {
    // while (1) {
        FD_SET(game->sockfd, game->active_fd_set);
        print_game_state(game);

        switch(game->server_state) {
            case RECEIVE: {
                if(game->game_state == IN_PLAY && is_round_over(game)) {
                    fprintf(stderr, "round is over! \n");
                    game->game_state   = END_OF_GAME;
                    game->server_state = SEND;
                    break;
                }

                bool timed_out = receive_data(game);

                if (game->game_state == LAUNCH) {
                    game->server_state = SEND;

                } else if (game->game_state == IN_PLAY && timed_out) {
                    game->server_state = UPDATE;
                }
                break;
            }
            
            case UPDATE: {
                game->server_state = SEND;
                // char *updated_data = update(game);
                // make_message
                break;
            }
            
            case SEND: {
                switch(game->game_state) {
                    case LAUNCH:
                        fprintf(stderr, "send start notification \n");
                        send_start_notification(game);
                        set_start_time(game);
                        game->game_state   = IN_PLAY;
                        game->server_state = RECEIVE;
                        break;

                    case IN_PLAY:
                        send_map(game);
                        game->server_state = RECEIVE;
                        break;

                    case END_OF_GAME:
                        fprintf(stderr, "end of game %d\n", game->round);
                        game->round++;
                        send_end_game_notifcation(game);
                        
                        // next round setup
                        if (game->num_registered_players < MAX_ACTIVE_PLAYERS) {
                            game->game_state   = WAITING;
                            game->server_state = RECEIVE;
                        
                        } else {
                            update_players(game);
                            start_game(game);
                            game->server_state = SEND;
                        }
                        
                        set_start_time(game); // todo: feels hacky...
                        break;
                }
                break;
            }
        }
        i++;
        fprintf(stderr, "i: %d\n", i);
    }
}

void update_players(Game game)
{
    Player curr = game->active_p_head;
    int i = 0;
    while (curr != NULL && i < MAX_ACTIVE_PLAYERS) {
        curr->player_state = SPECTATING;
        fprintf(stderr, "spectating: %s\n", curr->name);
        curr = curr->next;
        i++;

        if (curr == NULL) {
            curr = game->players_head;
        }
    }
    game->active_p_head = curr;
    fprintf(stderr, "active_p_head: %s\n", game->active_p_head->name);

}

// returns true if the current game round is over, i.e. n seconds elapsed.
// returns false otherwise.
bool is_round_over(Game game)
{   
    return (get_current_time() - time_in_billion(game)) / BILLION >= ROUND_TIME;
}

void send_start_notification(Game game)
{
    Message msg = malloc(sizeof(*msg));
    assert(msg != NULL);
    
    msg->type = 5;

    bzero(msg->id, PLAYER_NAME_LEN);
    strcpy(msg->id, "Server");

    bzero(msg->data, MAX_DATA_LEN);
    strcpy(msg->data, "map1");
    
    // minotaur is first one in group of n active players
    strcpy(msg->data + 32, game->active_p_head->name);
    fprintf(stderr, "minotaur: %s\n", msg->data + 32);

    int i;
    for (i = 0; i < 20; i++) {
        fprintf(stderr, "%c", msg->id[i]);
    }   
    
    fprintf(stderr, "\n* * * * * *\n");

    for (i = 0; i < 52; i++) {
        fprintf(stderr, "%c", msg->data[i]);
    }
    fprintf(stderr, "\n* * * \n");
    send_to_all(game, (char*)msg, sizeof(*msg));
    
    free(msg);
}

// # Players <char> (1 byte)
// Player 1 name <char*> (20 bytes)
// Player 1 score <int> (4 bytes)
// ...
// while loop that memcpy's 3 things at once i times, advances pointer in array by 24 bytes
void send_end_game_notifcation(Game game)
{
    Message msg = malloc(sizeof(*msg));
    assert (msg != NULL);
    msg->type = 6;
    
    bzero(msg->id, PLAYER_NAME_LEN);
    memcpy(msg->id, "Server", PLAYER_NAME_LEN);

    send_to_all(game, (char*) msg, sizeof(*msg));
}

// sends the updated coordinates to all the registered players
void send_map(Game game)
{
    char msg[] = "updated map";
    // create a Message struct and memcpy the data part stored in game->updated_data

    //TODO include correct message size
    // 4  = sequence num (int)
    // 1  = num players (char)
    // 22 = player name (20 chars) + x, y coordinates (1 char)
    int msg_size = 4 + 1 + game->num_active_players * 22;
    
    send_to_all(game, msg, 11);
    
}

// helper function to send a message to all registered players
void send_to_all(Game game, char *msg, int size) 
{
    fprintf(stderr, "MESSAGE\n");
    int i;
    for (i = 0; i < size; i++) {
        fprintf(stderr, "%c", msg[i]);

    }
    fprintf(stderr, "\nend of message\n");

    Player curr = game->players_head;

    while (curr != NULL) {   
        int bytes = sendto(game->sockfd, msg, size, 0, 
                           (struct sockaddr *) curr->player_addr, 
                           curr->addr_len);

        if (bytes < 0)
            fprintf(stderr, "ERROR in sendto: %d\n", bytes);

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

	if (select(FD_SETSIZE, game->active_fd_set, NULL, NULL, game->timeout) < 0) 
    {
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
    
    switch (buf[0]) {
        case REGISTER: {
            register_player(game, buf + PLAYER_NAME_INDEX, clientaddr,
                           clientlen);
            start_game(game);
            print_players(game);
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
        case LAUNCH:
            strcpy(game_state, "LAUNCH");
            break;
        
        default:
            strcpy(game_state, "ERROR");
            break;

    }
    fprintf(stderr, "Game State: %s\n", game_state);
}

// determines if the game can start based on the number of registered players
// and changes the game state to "IN_PLAY" if so
// and changes the state of two players to "PLAYING"
void start_game(Game game) 
{
    if (game->num_registered_players >= MAX_ACTIVE_PLAYERS) {
        Player curr = game->active_p_head;
        int i = 0;
        while (curr != NULL && i < MAX_ACTIVE_PLAYERS) {
             curr->player_state = PLAYING;
             fprintf(stderr, "playing: %s\n", curr->name);
             curr = curr->next;
             i++;

             if (curr == NULL) {
                curr = game->players_head;
             }
        }
        
        if (game->game_state != IN_PLAY) {
            game->game_state = LAUNCH;
        }
    }
}

// todo: might need better logic for initializing players
void initialize_game(Game game, int sockfd)
{
    // map
    game->map = NULL;

    // game rounds
    game->round = 0;
    game->start_time = malloc(sizeof *game->start_time);
    assert(game->start_time != NULL);
    set_start_time(game);
    
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

// set a round's start time to the current clock time.
void set_start_time(Game game)
{
    clock_gettime(CLOCK_REALTIME, game->start_time);
}

// converts the time to billions for easy comparisons.
int64_t time_in_billion(Game game)
{
    return BILLION * game->start_time->tv_sec + game->start_time->tv_nsec;
}

// returns the current time in billions
int64_t get_current_time()
{
    struct timespec t;
    clock_gettime(CLOCK_REALTIME, &t);

    return BILLION * t.tv_sec + t.tv_nsec;
}
















