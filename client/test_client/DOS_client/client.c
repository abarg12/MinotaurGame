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
#include <errno.h>
#include <signal.h>
#include <assert.h>
#include "client.h"

#define BUFSIZE 533

#define GHEIGHT 28
#define GWIDTH  96

char *map_name;
char *map;
int move_seq;
enum Role { HUMAN, MINOTAUR, SPECTATOR } role;
void registration_rq(ServerData *sd, char *player_name);
void generate_clients();
void spawn_clients(ServerData *sd);
void ack_loop(ServerData *sd);

char *itername;
char **client_name_list;  
int num_clients;

int main(int argc, char **argv) {
    ServerData sd;
    char buf[BUFSIZE];
    map_name = malloc(32);
    
   
    if (argc == 3) {
        sd.hostname = "comp112-05.cs.tufts.edu";
        sd.port_num = atoi(argv[1]); 
        num_clients = atoi(argv[2]);
        assert(num_clients > 0 && num_clients < 1000000);
    } else if (argc == 4) {
        sd.hostname = argv[3];
        sd.port_num = atoi(argv[1]); 
        num_clients = atoi(argv[2]);
        assert(num_clients > 0 && num_clients < 1000000);
    } else {
        fprintf(stderr, "usage: %s <port> <num_clients> (<host name>))\n", argv[0]);
        exit(1);
    }

    /* socket: create the socket */
    sd.sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sd.sockfd < 0) { 
        fprintf(stderr, "ERROR opening socket\n");
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

    generate_clients();
    spawn_clients(&sd); 
    ack_loop(&sd);

    return 0;
}


void registration_rq(ServerData *sd, char *player_name) {
    // create the buffer to send to server 
    char rrq[21];
    char buf[BUFSIZE];
    int n;
    rrq[0] = 0;
    memcpy(rrq + 1, player_name, 20); 

    // send the registration req message
    n = sendto(sd->sockfd, rrq, 21, 0, (struct sockaddr *) &(sd->serveraddr), sizeof(sd->serveraddr));
    if (n < 0) {
        fprintf(stderr, "sendto error for registration message\n");
        exit(1);
    }

    while (1) {
        // get the registration ACK to continue
        n = recvfrom(sd->sockfd, buf, BUFSIZE, 0, (struct sockaddr *) &sd->serveraddr, &sd->serverlen);
        return;
        
        /*if (buf[0] == 9) {
            //parse_reg_ack(buf);
            return;
        } 
       // else if (buf[0] == 5) {
       //     parse_start_info(buf, player_name);
       //     return;
       // }
        else {
            // TODO: uncomment code below and test its behavior (reg -> reg-ack) 
            fprintf(stderr, "First message from server was not Registration-Ack\n");
            exit(1);
            //continue;
        }*/
        
    }

    // Everyone starts as a spectator
    role = SPECTATOR;

    return;
}



void generate_clients() {
    client_name_list = malloc(8 * num_clients); 

    int n = 0;
    for (int i = 33; i < 127; i++) {
        for (int j = 33; j < 127; j++) {
            for (int k = 33; k < 127; k++) {
                for (int l = 33; l < 127; l++) {
                    if (n == num_clients) {
                        return;
                    }
                    char *new_name = malloc(4);
                    new_name[0] = i;
                    new_name[1] = j;
                    new_name[2] = k;
                    new_name[3] = l;

                    client_name_list[n] = new_name;

                    n = n + 1;
                }
            }
        }
    }
}

void spawn_clients(ServerData *sd) {
    for (int i = 0; i < num_clients; i++) {
        registration_rq(sd, client_name_list[i]);
    }
}

void ack_loop(ServerData *sd) {
    while (1) {
        printf("sending ping acks\n");
        for (int i = 0; i < num_clients; i++) {
            char ack_msg[21];
            ack_msg[0] = 8;
            memcpy(ack_msg + 1, client_name_list[i], 20);
            
            int n = sendto(sd->sockfd, ack_msg, 21, 0, (struct sockaddr *) &(sd->serveraddr), sizeof(sd->serveraddr));
            if (n < 0) {
                fprintf(stderr, "sendto error for ack\n");
                exit(1);
            }

        }
        sleep(1);
    }
}


