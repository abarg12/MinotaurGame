/*** client.c ***/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <ncurses.h>
#include "client.h"


PlayerState lobby_loop(char *hostname, int port_num);


int main(int argc, char **argv) {
    char *hostname;
    int port_num;

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
