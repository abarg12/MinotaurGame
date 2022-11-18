/*** client.c ***/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <ncurses.h>
#include <sys/types.h>
#include <netdb.h> 
#include "client.h"

#define BUFSIZE 533
#define GHEIGHT 28
#define GWIDTH  96

PlayerState lobby_loop(ServerData *sd, WINDOW *game_window, char *player_name);
void client_exit(WINDOW *game_window);
void registration_rq(ServerData *sd, char *player_name);
void download_map(char *map_name);
void draw_map(WINDOW *game_window);
PlayerState play_loop(ServerData *sd, WINDOW *game_window, char *player_name);
int parse_instr(ServerData *sd, char *player_name);
void send_mv_inst(ServerData *sd, char *player_name, char move_type);

char *map;
int move_seq;

int main(int argc, char **argv) {
/*    
    char *hostname;
    int port_num;
    int sockfd;
    int serverlen;
    struct sockaddr_in serveraddr;
    struct hostent *server;
*/
    ServerData sd;
    char buf[BUFSIZE];
    char *player_name = malloc(20);
    
    if (argc == 1) {
        sd.hostname = "comp112-05.cs.tufts.edu";
        sd.port_num = 9040;
    } else if (argc == 3) {
        sd.hostname = argv[1];
        sd.port_num = atoi(argv[2]); 
    } else {
        fprintf(stderr, "usage: %s <server address> <port>\n", argv[0]);
        exit(1);
    }

    /* socket: create the socket */
    sd.sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sd.sockfd < 0) { 
        fprintf(stderr, "ERROR opening socket");
        exit(1);
    }

    /* gethostbyname: get the server's DNS entry */
    sd.server = gethostbyname(sd.hostname);
    if (sd.server == NULL) {
        fprintf(stderr,"ERROR, no such host as %s\n", sd.hostname);
        exit(1);
    }

    /* build the server's Internet address */
    bzero((char *) &(sd.serveraddr), sizeof(sd.serveraddr));
    sd.serveraddr.sin_family = AF_INET;
    bcopy((char *) sd.server->h_addr, 
    (char *)&(sd.serveraddr.sin_addr.s_addr), sd.server->h_length);
    sd.serveraddr.sin_port = htons(sd.port_num);
    sd.serverlen = sizeof(sd.serveraddr);


    // start ncurses mode
    initscr();
    curs_set(2);
    refresh();
    WINDOW *game_window;
    start_color();
    init_pair(1, COLOR_WHITE, COLOR_WHITE);
    init_pair(2, COLOR_RED, COLOR_BLACK);

    PlayerState pstate = IN_LOBBY; 
    int sentinel = 1;
    while (sentinel) {
        switch (pstate) {
            case IN_LOBBY: {
                pstate = lobby_loop(&sd, game_window, player_name);
                break;
            }

            case PLAYING: {
                map = malloc(GWIDTH * GHEIGHT);
                // TODO: make this dynamic
                download_map("../maps/map1");                
                draw_map(game_window);
                pstate = play_loop(&sd, game_window, player_name); 
                sentinel = 0;
                break;
            }

            default: {
                fprintf(stderr, "case not handled\n");
                client_exit(game_window);
                exit(1);
            }
        }
    }
client_exit(game_window);
    return 0;
}


PlayerState play_loop(ServerData *sd, WINDOW *game_window, char *player_name) {
    fd_set active_fd_set, read_fd_set;    
    FD_ZERO(&active_fd_set);
    FD_SET(0, &active_fd_set);
    FD_SET(sd->sockfd, &active_fd_set);
    char buf[BUFSIZE];
    int n;
    move_seq = 0;


    int game_in_progress = 1;
    while (game_in_progress) {
        read_fd_set = active_fd_set;
        
        // The game select loop blocks until user input or map is received
        if (select(FD_SETSIZE, &read_fd_set, NULL, NULL, NULL) < 0) {
            fprintf(stderr, "select error\n");
            exit(1);
        }
    
        int i;
        for (i = 0; i < FD_SETSIZE; i++) {

            if (FD_ISSET (i, &read_fd_set)) {
                if (i == 0) {
                    // exit when parse int returns -1
                    if (parse_instr(sd, player_name) < 0) {
                        client_exit(game_window);
                        exit(0);
                    }
                }
                else {
                    // TODO: map data received, print it out
                    n = recvfrom(sd->sockfd, buf, BUFSIZE, 0, (struct sockaddr *) &sd->serveraddr, &sd->serverlen);
                }
            }
        } 
    }
}


int parse_instr(ServerData *sd, char *player_name) {
    // stdin received
    int len, n;
    char instr[50];
    len = read(0, instr, 50);

    char key = instr[len - 1]; 
    move(0,0);
    clrtoeol();
    printw("%d: %d", len, key);
    refresh(); 

    // return -1 if client presses 'escape' (27) or 'q' (113)
    if (key == 27 || key == 113) {
        return -1;
    } else if (key == 65 || key == 119) {
        // case for up arrow or 'w'
        send_mv_inst(sd, player_name, 0); 
    } else if (key == 67 || key == 100) {
        // case for right arrow or 'd'
        send_mv_inst(sd, player_name, 1); 
    } else if (key == 66 || key == 115) {
        // case for down arrow or 's'
        send_mv_inst(sd, player_name, 2); 
    } else if (key == 68 || key == 97) {
        // case for left arrow or 'a'
        send_mv_inst(sd, player_name, 3); 
    } else {
        return -1;
    }
}

void send_mv_inst(ServerData *sd, char *player_name, char move_type) {
        int seq_no = htonl(move_seq);

        InstrStruct st_msg;
        st_msg.type = 2;
        bzero(st_msg.ID, 20);
        memcpy(st_msg.ID, player_name, 20);
        bzero(st_msg.DATA, 512);
        memcpy(st_msg.DATA, &seq_no, sizeof(seq_no));
        st_msg.DATA[sizeof(seq_no)] = move_type;
        
        char *msg_arr;
        msg_arr = (char *) &st_msg;

        int n;
        n = sendto(sd->sockfd, msg_arr, 533, 0, (struct sockaddr *) &(sd->serveraddr), sizeof(sd->serveraddr));
        if (n < 0) {
            fprintf(stderr, "error in sending move instruction\n");
            exit(1);
        }
}


PlayerState lobby_loop(ServerData *sd, WINDOW *game_window, char *player_name) {
    // Set up lobby UI
    game_window = newwin(GHEIGHT, GWIDTH, 3, 0);
    mvwprintw(game_window, 2, 3, "Minotaur Lobby\n");
    mvwprintw(game_window, 4, 3, "Connected to server with address: %s\n", sd->hostname); 
    //mvwprintw(game_window, 5, 3, "screensize = %d, %d\n", LINES, COLS);
    box(game_window, 0, 0);
    mvprintw(0,0, "Please enter name: "); 
    wrefresh(game_window);
    refresh();

    // Accept user input
    char input_buf[100];
    getstr(input_buf); 
    strncpy(player_name, input_buf, 19);
    player_name[19] = '\0'; 

    move(0,0);
    clrtoeol();
    mvprintw(0,0, "Welcome, %s", player_name);
    mvprintw(1,0, "Press 's' to join game queue or 'q' to quit");
    curs_set(0);
    noecho();
    refresh();
    
    int c = getch();
    while ((c != (int) 's') && (c != (int) 'q')) {
       c = getch(); 
    }

    if (c == (int) 'q') {
        client_exit(game_window);
        exit(0);
    } else {
        mvwprintw(game_window, 6, 3, "Sent request to game server");
        mvwprintw(game_window, 7, 3, "Waiting for game to start");
        wrefresh(game_window);
        registration_rq(sd, player_name);
        delwin(game_window);
        return PLAYING;
    }
    
    delwin(game_window);
    return IN_LOBBY;
}


void registration_rq(ServerData *sd, char *player_name) {
    // create the buffer to send to server 
    char rrq[21];
    char buf[BUFSIZE];
    int n;
    rrq[0] = 0;
    memcpy(rrq + 1, player_name, 20); 

    n = sendto(sd->sockfd, rrq, 21, 0, (struct sockaddr *) &(sd->serveraddr), sizeof(sd->serveraddr));
    if (n < 0) {
        fprintf(stderr, "sendto error\n");
        exit(1);
    }

    // TODO: connect to server to get start signal
    buf[0] = 5;
    //n = recvfrom(sd->sockfd, buf, BUFSIZE, 0, (struct sockaddr *) &sd->serveraddr, &sd->serverlen);

    // Get game start notification
    if (buf[0] != 5) {
        fprintf(stderr, "Was expecting Game Start msg from server but got something else\n");
        exit(1);
    }

    return;
}


// TODO: figure out a way to clear up all memory when closing
//  i.e. player name and other malloc'd data
void client_exit(WINDOW *game_window) {
    delwin(game_window);
    endwin();
}


void download_map(char *map_name) {
    FILE *fptr = fopen(map_name, "rb");
    if (fptr == NULL) {
        fprintf(stderr, "Map does not exist at specified location\n");
        exit(1);
    }

    int n = fread(map, 1, GWIDTH * GHEIGHT, fptr);
    fclose(fptr);
}


void draw_map(WINDOW *game_window) {
    game_window = newwin(GHEIGHT, GWIDTH, 3, 0);

    int x, y;
    char val;
    int minotaurx = 45;
    int minotaury = 13;
    for (y = 0; y < GHEIGHT; y++) {
        wmove(game_window, y, 0);
        for (x = 0; x < GWIDTH; x++) {
           val = map[x + (GWIDTH * y)];
           if (x == minotaurx && y == minotaury) {
               wattron(game_window, COLOR_PAIR(2));
               waddch(game_window, 'M');
               wattroff(game_window, COLOR_PAIR(2));
           } else if (val == '1') {
               wattron(game_window, COLOR_PAIR(1));
               waddch(game_window, '.');
               wattroff(game_window, COLOR_PAIR(1));
           } else if (val == '0') {
               waddch(game_window, ' '); 
           }
        }
    }
    wrefresh(game_window);
}

