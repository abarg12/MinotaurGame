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

PlayerState lobby_loop(char *hostname, int port_num);


int main(int argc, char **argv) {
    char *hostname;
    int port_num;
    int sockfd;
    int serverlen;
    struct sockaddr_in serveraddr;
    struct hostent *server;
    char buf[BUFSIZE];
    
    if (argc == 1) {
        hostname = "comp112-05.cs.tufts.edu";
        port_num = 9051;
    } else if (argc == 3) {
        hostname = argv[1];
        port_num = atoi(argv[2]); 
    } else {
        fprintf(stderr, "usage: %s <server address> <port>\n", argv[0]);
        exit(1);
    }

    /* socket: create the socket */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) { 
        fprintf(stderr, "ERROR opening socket");
        exit(1);
    }

    /* gethostbyname: get the server's DNS entry */
    server = gethostbyname(hostname);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host as %s\n", hostname);
        exit(1);
    }

    /* build the server's Internet address */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
    (char *)&serveraddr.sin_addr.s_addr, server->h_length);
    serveraddr.sin_port = htons(port_num);


    // start ncurses mode
    initscr();

    PlayerState pstate = IN_LOBBY; 
    while (1) {
        switch (pstate) {
            case IN_LOBBY: {
                pstate = lobby_loop(hostname, port_num);
                break;
            }
            
            default: {
                fprintf(stderr, "case not handled\n");
                exit(1);
            }
        }
    }

    endwin();
    return 0;
}

PlayerState lobby_loop(char *hostname, int port_num) {
    printw("Lobby\n");
    printw("Connected to server with address: %s\n", hostname); 
    refresh();
    getch(); 
}
