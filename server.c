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

	if (argc != 3) {
		fprintf(stderr, "usage: %s <round time> <file name>\n", argv[0]);
		exit(1);
	}
	portno = 9040;
    ROUND_TIME = atoi(argv[1]);
    char map_name[MAP_NAME_LEN];
    strcpy(map_name, argv[2]);

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

    initialize_game(game, sockfd, map_name);
    int i= 0;
    // while(i < 40) {
    while (1) {
        FD_SET(game->sockfd, game->active_fd_set);
        print_game_state(game);

        switch(game->server_state) {
            case RECEIVE: {
                if(game->game_state == IN_PLAY && is_round_over(game)) {
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
                update(game);
                game->server_state = SEND;
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
                        // fprintf(stderr, "sending map\n");
                        send_map(game);
                        game->server_state = RECEIVE;
                        break;

                    case END_OF_GAME:
                        fprintf(stderr, "end of game %d notification\n",game->round);
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
                        
                        set_start_time(game);
                        break;
                }
                break;
            }
        }
        // i++; fprintf(stderr, "i: %d\n", i);
    }
}

void update_players(Game game)
{
    Player curr = game->active_p_head;
    int i = 0;
    while (curr != NULL && i < MAX_ACTIVE_PLAYERS) {
        curr->player_state = SPECTATING;
        game->num_active_players--;
        // fprintf(stderr, "spectating: %s\n", curr->name);
        curr = curr->next;
        i++;

        if (curr == NULL) {
            curr = game->players_head;
        }
    }
    game->active_p_head = curr;
}

// returns true if the current game round is over, i.e. n seconds elapsed.
// returns false otherwise.
bool is_round_over(Game game)
{   
    return (get_current_time() - time_in_billion(game)) / BILLION >= ROUND_TIME;
}

// includes
// Map name                 <char *> (32 bytes)
// Number of active players <char>   (1 byte)
// Player1 (Minotaur) name  <char *> (20 bytes)
// Player2 name             <char *> (20 bytes)
void send_start_notification(Game game)
{
    Message msg = malloc(sizeof(*msg));
    assert(msg != NULL);
    
    msg->type = GAME_START_NOTIFICATION;

    bzero(msg->id, PLAYER_NAME_LEN);
    strcpy(msg->id, "Server");

    bzero(msg->data, MAX_DATA_LEN);
    strcpy(msg->data, game->map_name);
    // fprintf(stderr, "map_name %s\n", game->map_name);
    
    msg->data[MAP_NAME_LEN] = game->num_active_players;
    // fprintf(stderr, "num active players: %d\n",  msg->data[MAP_NAME_LEN]);

    // add all the active players 
    char *j = msg->data + MAP_NAME_LEN + 1; // +1 to go past num players
    add_active_players(game, j);
  
    send_to_all(game, (char*)msg, sizeof(*msg));
    
    free(msg);
}

// adds the names of all the active players into array pointed to by j.
void add_active_players(Game game, char *j) 
{
    Player p = game->players_head;
    while (p != NULL) {
        if (p->player_state == PLAYING) {
            strcpy(j, p->name);
            j += PLAYER_NAME_LEN;
        }
        p = p->next;
    }
}

// # Players <char> (1 byte)
// Player 1 name <char*> (20 bytes)
// Player 1 score <int> (4 bytes)
// ...
void send_end_game_notifcation(Game game)
{
    Message msg = malloc(sizeof(*msg));
    assert (msg != NULL);
    msg->type = END_OF_GAME_NOTIFICATION;
    
    bzero(msg->id, PLAYER_NAME_LEN);
    memcpy(msg->id, "Server", PLAYER_NAME_LEN);

    bzero(msg->data, MAX_DATA_LEN);
    msg->data[0] = game->num_active_players;

    add_names_scores(game, msg->data + 1);

    send_to_all(game, (char*) msg, sizeof(*msg));
}

// helper function to add player names followed by their score for the
// end of game notification
// Player 1 name <char*> (20 bytes)
// Player 1 score <int> (4 bytes)
// n times, n = number of active players
void add_names_scores(Game game, char *msg)
{
    int i = 0;
    Player curr = game->active_p_head;
    while (i < game->num_active_players)
    {   
        if (curr->player_state == PLAYING) {
            // add name
            memcpy(msg, curr->name, PLAYER_NAME_LEN);
            
            // add score
            memcpy(msg + PLAYER_NAME_LEN, &curr->score, sizeof(int));

            // advance pointer in message
            msg += PLAYER_NAME_LEN + sizeof(int);
        }

        curr = curr->next;

        // loop back around
        if (curr == NULL) {
            curr = game->players_head;
        }
        i++;
    }
}

// send the player a registration ack after they've been added to the
// players list.
void send_player_registration_ack(Game game, Player p)
{
    Message msg = malloc(sizeof(*msg));
    assert (msg != NULL);
    
    msg->type = REGISTRATION_ACK;

    bzero(msg->id, PLAYER_NAME_LEN);
    memcpy(msg->id, "Server", PLAYER_NAME_LEN);

    bzero(msg->data, MAX_DATA_LEN);
    memcpy(msg->data, game->map_name, MAP_NAME_LEN);

    send_to_single(game, p, (char*) msg, sizeof(*msg));
}

// sends the updated coordinates to all the registered players
void send_map(Game game)
{
    Message msg = malloc(sizeof(*msg));
    assert (msg != NULL);
    msg->type = CURRENT_MAP;
    
    bzero(msg->id, PLAYER_NAME_LEN);
    strcpy(msg->id, "Server");

    bzero(msg->data, MAX_DATA_LEN);
    int data_size = 4 + 1 + game->num_active_players * 22;
   
    memcpy(msg->data, game->update_to_send, data_size);
    // TODO: change so not hardcoded value 21 for header size: 21 + msg_size
    send_to_all(game, (char*) msg, sizeof(*msg)); 
}

// helper function to send a message to all registered players
void send_to_all(Game game, char *msg, int size) 
{
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

// sends a message of size bytes to player p.
void send_to_single(Game game, Player p, char *msg, int size)
{
    int bytes = sendto(game->sockfd, msg, size, 0, 
                      (struct sockaddr *) p->player_addr, 
                       p->addr_len);
    if (bytes < 0)
        fprintf(stderr, "ERROR in sendto: %d\n", bytes);
}

void reset_timeout(Game game)
{
    game->timeout->tv_sec = 0;
    // game->timeout->tv_sec = 1;
    // 500,000 microseconds = 0.5 seconds
    game->timeout->tv_usec = 250000;
    // game->timeout->tv_usec = 0;
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
            if (game->game_state == WAITING) {
                start_game(game);
            }
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
    // fprintf(stderr, "Game State: %s\n", game_state);
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
             if (i == 0) {
                curr->phys.x = MWIDTH/2;
                curr->phys.y = MHEIGHT/2;
                curr->phys.d = DOWN;
             } else {
                // The upper-left corner of the screen
                curr->phys.x = 6;
                curr->phys.y = 3;
                curr->phys.d = UP;

             }
             game->num_active_players++;
            //  fprintf(stderr, "playing: %s\n", curr->name);
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

void initialize_game(Game game, int sockfd, char *file_name)
{
    // map
    bzero(game->map_name, MAP_NAME_LEN);
    strcpy(game->map_name, file_name);
    game->map = NULL;
    game->update_to_send = NULL;

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
    // game->timeout->tv_sec = 1;
    // game->timeout->tv_usec = 0;
    reset_timeout(game);

    load_map(file_name, game);
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
















