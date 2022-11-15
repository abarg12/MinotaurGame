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

PlayerState lobby_loop(ServerData *sd, WINDOW *game_window, char *player_name);
void client_exit(WINDOW *game_window);
void registration_rq(ServerData *sd, char *player_name);


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

    PlayerState pstate = IN_LOBBY; 
    int sentinel = 1;
    while (sentinel) {
        switch (pstate) {
            case IN_LOBBY: {
                pstate = lobby_loop(&sd, game_window, player_name);
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

PlayerState lobby_loop(ServerData *sd, WINDOW *game_window, char *player_name) {
    // Set up lobby UI
    game_window = newwin(32, 96, 3, 0);
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
        registration_rq(sd, player_name);
        mvwprintw(game_window, 6, 3, "Sent request to game server");
        wrefresh(game_window);
        sleep(1);
        client_exit(game_window);
        exit(0);
    }
    
    delwin(game_window);
    return IN_LOBBY;
}


void registration_rq(ServerData *sd, char *player_name) {
    // create the buffer to send to server 
    char rrq[21];
    char buf[533];
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
    // n = recvfrom(sd.sockfd, buf, BUFSIZE, 0, (struct sockaddr *) &sd.serveraddr, &sd.serverlen);
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

