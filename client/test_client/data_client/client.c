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
#include <time.h>
#include "client.h"

#define BUFSIZE 533

#define GHEIGHT 28
#define GWIDTH  96

void client_exit(WINDOW *game_window);
void registration_rq(ServerData *sd, char *player_name);
void download_map();
void draw_map(WINDOW *game_window);
void reset_map(WINDOW *game_window);
void update_map(char *buf, WINDOW *game_window, char *player_name);
PlayerState lobby_loop(ServerData *sd, WINDOW *game_window, char *player_name);
PlayerState play_loop(ServerData *sd, WINDOW *game_window, char *player_name);
PlayerState end_game_loop(ServerData *sd, WINDOW *game_window, char *player_name);
int  parse_instr(ServerData *sd, char *player_name);
void send_mv_inst(ServerData *sd, char *player_name, char move_type);
void send_exit_msg(ServerData *sd, char *player_name);
void print_buffer(char *buf, int n);
void parse_start_info(char *buf, char *player_name);
int  parse_reg_ack(char *buf);
void ack_ping(ServerData *sd, char *player_name);

char *map_name;
char *map;
int move_seq;
enum Role { HUMAN, MINOTAUR, SPECTATOR } role;
char *scoreBuf;
FILE *logfile;
struct timespec t1;
struct timespec t2;

void intHandler(int signal) {
    client_exit(NULL);
    fprintf(stderr, "/\\/\\/\\/\\  client ungraceful exit  /\\/\\/\\/\\\n");
    exit(0);
}

int main(int argc, char **argv) {
    t1.tv_sec = 0;
    t1.tv_nsec = 0;
    t2.tv_sec = 0;
    t2.tv_nsec = 0;

    ServerData sd;
    char buf[BUFSIZE];
    char *player_name = malloc(20);
    map_name = malloc(32);
    
   
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

    // start ncurses mode
    initscr();
    signal(SIGINT, intHandler);
    curs_set(2);
    refresh();
    WINDOW *game_window = NULL;
    start_color();
    init_pair(1, COLOR_WHITE, COLOR_WHITE); // wall color (foreground and background white)
    init_pair(2, COLOR_RED, COLOR_BLACK);  // Minotaur text (foreground red, background black)
    init_pair(3, COLOR_GREEN, COLOR_BLACK);
    init_pair(4, COLOR_RED, COLOR_RED);
    init_pair(5, COLOR_GREEN, COLOR_GREEN);
    init_pair(6, COLOR_BLUE, COLOR_BLACK);
    init_pair(7, COLOR_BLACK, COLOR_GREEN);
    init_pair(8, COLOR_BLACK, COLOR_RED);

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
                download_map();
                draw_map(game_window);
                logfile = fopen("logfile.txt", "w");
                pstate = play_loop(&sd, game_window, player_name); 
                free(map);
                break;
            }

            case END_OF_GAME: {
                pstate = end_game_loop(&sd, game_window, player_name);                 
                break;
            }

            default: {
                client_exit(game_window);
                fprintf(stderr, "switch case not handled, AKA state machine fail\n");
                exit(1);
            }
        }
    }
    client_exit(game_window);
    return 0;
}

PlayerState end_game_loop(ServerData *sd, WINDOW *game_window, char *player_name) {
    fd_set active_fd_set, read_fd_set;    
    FD_ZERO(&active_fd_set);
    FD_SET(0, &active_fd_set);
    FD_SET(sd->sockfd, &active_fd_set);
    char buf[BUFSIZE];
    bzero(buf, BUFSIZE);
    int n, seconds;


    if (game_window != NULL) {
        delwin(game_window);
    }
    game_window = newwin(GHEIGHT, GWIDTH, 3, 0);

    move(0,0);
    clrtoeol();

    move(1,0);
    clrtoeol();
    printw("GAME OVER, waiting for new game to start");

    move(2,0);
    clrtoeol();
    printw("Press q to quit");

    wrefresh(game_window);
    refresh();

    assert(scoreBuf != NULL);
    int score_entries = scoreBuf[0]; 
    int score_offset = 1;
    char *score_player_name[20];
    int score;

    wmove(game_window, 2, 0);
    wprintw(game_window, "SCORES");
    wmove(game_window, 3, 0);
    wprintw(game_window, "-------------------");
    int screen_y_offset = 4;
    for (int i = 0; i < score_entries; i++) {
        memcpy(score_player_name, scoreBuf + score_offset, 20);
        memcpy(&score, scoreBuf + score_offset + 20, 4);
        score = ntohl(score);

        wmove(game_window, screen_y_offset + i, 0);
        wprintw(game_window, "%s", score_player_name);
        wprintw(game_window, ": %d", score);
        wrefresh(game_window);

        score_offset = score_offset + 24;
    } 
    
    // free the score buffer so it can be re-used later
    free(scoreBuf);

    while(1) {
        read_fd_set = active_fd_set;
        
        // The game select loop blocks until user input or map is received
        if (select(FD_SETSIZE, &read_fd_set, NULL, NULL, NULL) < 0) {
            if (errno == 4) {
                continue;
            }
            client_exit(game_window);
            exit(1);
        }
    
        int i;
        for (i = 0; i < FD_SETSIZE; i++) {

            if (FD_ISSET (i, &read_fd_set)) {
                if (i == 0) {
                    // parse the key sent by user 
                    char instr[50];
                    int len = read(0, instr, 50);
                    char key = instr[len - 1]; 

                    if (key == 27 || key == 113) {
                        send_exit_msg(sd, player_name);
                        client_exit(game_window);
                        fprintf(stderr, "------  client graceful exit  ------\n");
                        exit(0);
                    }
                }
                else {
                    // interpret messages from server 
                    n = recvfrom(sd->sockfd, buf, BUFSIZE, 0, (struct sockaddr *) &sd->serveraddr, &sd->serverlen);
                    if (buf[0] == 7) {
                        seconds = buf[21];
                        ack_ping(sd, player_name);
                    }
                    else if (buf[0] == 5) {
                        parse_start_info(buf, player_name);
                        return PLAYING;
                    }
                    else {
                        client_exit(game_window);
                        fprintf(stderr, "error due to message type set as: %d while in end of game mode\n", buf[0]);
                        exit(1);
                    }
                }
            }
        }
    }
}


PlayerState play_loop(ServerData *sd, WINDOW *game_window, char *player_name) {
    fd_set active_fd_set, read_fd_set;    
    FD_ZERO(&active_fd_set);
    FD_SET(0, &active_fd_set);
    FD_SET(sd->sockfd, &active_fd_set);
    char buf[BUFSIZE];
    bzero(buf, BUFSIZE);
    int n, seconds;
    move_seq = 0;

    move(0,0);
    clrtoeol();
    printw("GAME IN PROGRESS using map named: ");
    printw(map_name);
    
    move(2,0);
    clrtoeol();
    move(1,0);
    clrtoeol();
    printw("You are a ");
    if (role == HUMAN) {
        attron(COLOR_PAIR(3));
        printw("HUMAN");
        attroff(COLOR_PAIR(3));
    } else if (role == MINOTAUR) {
        attron(COLOR_PAIR(2));
        printw("MINOTAUR"); 
        attroff(COLOR_PAIR(2));
    } else {
        attron(COLOR_PAIR(6));
        printw("SPECTATOR");
        attroff(COLOR_PAIR(6));
    }
    refresh();

    int game_in_progress = 1;
    while (game_in_progress) {
        read_fd_set = active_fd_set;
        
        // The game select loop blocks until user input or map is received
        if (select(FD_SETSIZE, &read_fd_set, NULL, NULL, NULL) < 0) {
            // fprintf(stderr, "ERRNO: %d, select error\n", errno);
            if (errno == 4) {
                continue;
            }
            client_exit(game_window);
            exit(1);
        }
    
        int i;
        for (i = 0; i < FD_SETSIZE; i++) {

            if (FD_ISSET (i, &read_fd_set)) {
                if (i == 0) {
                    // exit when parse int returns -1
                    if (parse_instr(sd, player_name) < 0) {
                        client_exit(game_window);
                        fprintf(stderr, "------  client graceful exit  ------\n");
                        exit(0);
                    }
                }
                else {
                    // interpret messages from server 
                    n = recvfrom(sd->sockfd, buf, BUFSIZE, 0, (struct sockaddr *) &sd->serveraddr, &sd->serverlen);
                    if (buf[0] == 3) {
                        if ((t1.tv_nsec == 0) && (t1.tv_sec == 0)) {
                             clock_gettime(CLOCK_REALTIME, &t1);
                        } else {
                            clock_gettime(CLOCK_REALTIME, &t2);
                            int diff = 0;

                            // account for the fact that timespecs are represented
                            // as a struct with seconds and nanoseconds 
                            if (t2.tv_sec > t1.tv_sec) {
                                diff = diff + t2.tv_nsec;
                                diff = diff + (999999999-t1.tv_nsec);
                            } else {
                                diff = t2.tv_nsec - t1.tv_nsec;
                            }

                            t1.tv_sec = t2.tv_sec;
                            t1.tv_nsec = t2.tv_nsec;

                            char outstring[20];
                            sprintf(outstring, "%d\n", diff);
                            fputs(outstring, logfile);
                        }
                        update_map(buf, game_window, player_name);
                    }
                    else if (buf[0] == 6) {
                        // copy over the score info into a malloc'd buffer
                        scoreBuf = malloc(n - 21);
                        assert(scoreBuf != NULL);
                        memcpy(scoreBuf, buf + 21, n - 21);
                        return END_OF_GAME;
                    }
                    else if (buf[0] == 7) {
                        seconds = buf[21];
                        move(0,0);
                        clrtoeol();
                        printw("Time Remaining: %d", seconds);
                        refresh();
                        ack_ping(sd, player_name);
                    }
                    else if (buf[0] == 5) {
                        client_exit(game_window);
                        fprintf(stderr, "Game start notification interrupted an active game\n");
                        exit(1);

                    }
                    else {
                        client_exit(game_window);
                        fprintf(stderr, "error due to message type set as: %d\n", buf[0]);
                        exit(1);
                    }
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
    //move(0,0);
    //clrtoeol();
    //printw("%d: %d", len, key);
    //refresh(); 

    // return -1 if client presses 'escape' (27) or 'q' (113)
    if (key == 27 || key == 113) {
        send_exit_msg(sd, player_name);
        return -1;
    } else if (key == 65 || key == 119) {
        // case for up arrow or 'w'
        send_mv_inst(sd, player_name, 0); 
        move_seq = move_seq + 1;
    } else if (key == 67 || key == 100) {
        // case for right arrow or 'd'
        send_mv_inst(sd, player_name, 1); 
        move_seq = move_seq + 1;
    } else if (key == 66 || key == 115) {
        // case for down arrow or 's'
        send_mv_inst(sd, player_name, 2); 
        move_seq = move_seq + 1;
    } else if (key == 68 || key == 97) {
        // case for left arrow or 'a'
        send_mv_inst(sd, player_name, 3); 
        move_seq = move_seq + 1;
    } else {
        // TODO: the rest of the cases based on user input
    }
}


void send_exit_msg(ServerData *sd, char *player_name) {
    InstrStruct st_msg;
    st_msg.type = 1;
    bzero(st_msg.ID, 20);
    memcpy(st_msg.ID, player_name, 20);

    char *msg_arr;
    msg_arr = (char *) &st_msg;
    
    int n;
    n = sendto(sd->sockfd, msg_arr, 533, 0, (struct sockaddr *) &(sd->serveraddr), sizeof(sd->serveraddr));
    if (n < 0) {
            fprintf(stderr, "error in sending move instruction\n");
            exit(1);
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
            client_exit(NULL);
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
        fprintf(stderr, "------  client graceful exit  ------\n");
        exit(0);
    } else {
        mvwprintw(game_window, 6, 3, "Sending request to join game server");
        mvwprintw(game_window, 7, 3, "Waiting on server response...");
        wrefresh(game_window);
        registration_rq(sd, player_name);
        delwin(game_window);

        
        int n;
        char buf[BUFSIZE];
        while(1) {
            n = recvfrom(sd->sockfd, buf, BUFSIZE, 0, (struct sockaddr *) &sd->serveraddr, &sd->serverlen);

            if (buf[0] == 5) {
                parse_start_info(buf, player_name);
                break;
            } else if (buf[0] == 7) {
                ack_ping(sd, player_name);
            } else {
                break;
            }
        }

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

    
    // Everyone starts as a spectator
    role = SPECTATOR;


    // send the registration req message
    n = sendto(sd->sockfd, rrq, 21, 0, (struct sockaddr *) &(sd->serveraddr), sizeof(sd->serveraddr));
    if (n < 0) {
        client_exit(NULL);
        fprintf(stderr, "sendto error for registration message\n");
        exit(1);
    }

    while (1) {
        // get the registration ACK to continue
        n = recvfrom(sd->sockfd, buf, BUFSIZE, 0, (struct sockaddr *) &sd->serveraddr, &sd->serverlen);
        
        if (buf[0] == 9) {
            if (parse_reg_ack(buf) == '1') {
                return;
            } else {
                client_exit(NULL);
                fprintf(stderr, "Client name already in use, please choose another\n");
                exit(0);
            }
        } 
       // else if (buf[0] == 5) {
       //     parse_start_info(buf, player_name);
       //     return;
       // }
        else {
            client_exit(NULL);
            fprintf(stderr, "First message from server was not Registration-Ack\n");
            exit(1);
            //continue;
        }
        
    }

    return;
}


// TODO: figure out a way to clear up all memory when closing
//  i.e. player name and other malloc'd data
void client_exit(WINDOW *game_window) {
    if (game_window != NULL) {
        delwin(game_window);
    }
    if (map_name != NULL) {
        free(map_name);
    }
    fclose(logfile);
    endwin();
}


void download_map() {
    char file_location[60];
    bzero(file_location, 60);
    strcat(file_location, "../../../");
    strcat(file_location, map_name);
    //fprintf(stderr, "file location: \n%s\n", file_location);
    FILE *fptr = fopen(file_location, "rb");
    if (fptr == NULL) {
        fprintf(stderr, "Map does not exist at specified location in files\n");
        exit(1);
    }

    int n = fread(map, 1, GWIDTH * GHEIGHT, fptr);
    fclose(fptr);
}


void draw_map(WINDOW *game_window) {
    game_window = newwin(GHEIGHT, GWIDTH, 3, 0);

    int x, y;
    char val;
    for (y = 0; y < GHEIGHT; y++) {
        wmove(game_window, y, 0);
        for (x = 0; x < GWIDTH; x++) {
           val = map[x + (GWIDTH * y)];
           if (val == '1') {
               wattron(game_window, COLOR_PAIR(1));
               waddch(game_window, ' ');
               wattroff(game_window, COLOR_PAIR(1));
           } else if (val == '0') {
               waddch(game_window, ' '); 
           }
        }
    }
    wrefresh(game_window);
}


void reset_map(WINDOW *game_window) {
    delwin(game_window);
    game_window = newwin(GHEIGHT, GWIDTH, 3, 0);

    int x, y;
    char val;
    for (y = 0; y < GHEIGHT; y++) {
        wmove(game_window, y, 0);
        for (x = 0; x < GWIDTH; x++) {
           val = map[x + (GWIDTH * y)];
           if (val == '1') {
               wattron(game_window, COLOR_PAIR(2));
               waddch(game_window, '.');
               wattroff(game_window, COLOR_PAIR(2));
           } else if (val == '0') {
               waddch(game_window, ' '); 
           }
        }
    }
    wrefresh(game_window);
}


void print_buffer(char *buf, int n) {
    int i;
    move(0,0);
    clrtoeol();
    
    InstrStruct *msg_struct = malloc(533);
    msg_struct = (InstrStruct *) buf;


    printw("Msg type: %d  ", msg_struct->type);
    printw("Name: %s  ", msg_struct->ID);
    printw("Data: ");
    int data_len = n;
    for (i = 26; i < data_len; i++) {
         printw("%c", buf[i]);
    }

    refresh();
}


void update_map(char *buf, WINDOW *game_window, char *player_name) {
    if (game_window != NULL) {
        delwin(game_window);
    }
    game_window = newwin(GHEIGHT, GWIDTH, 3, 0);

    int x, y;
    char val;
    for (y = 0; y < GHEIGHT; y++) {
        wmove(game_window, y, 0);
        for (x = 0; x < GWIDTH; x++) {
           val = map[x + (GWIDTH * y)];
           if (val == '1') {
               wattron(game_window, COLOR_PAIR(1));
               waddch(game_window, ' ');
               wattroff(game_window, COLOR_PAIR(1));
           } else if (val == '0') {
               waddch(game_window, ' '); 
           }
        }
    }
        
    //TODO: test parsing buffer with multiple human players
    int minotaurx = buf[46];
    int minotaury = buf[47];

    wmove(game_window, minotaury, minotaurx);
    if (strcmp(buf + 26, player_name)) {
        wattron(game_window, COLOR_PAIR(4));
        waddch(game_window,' ');
        waddch(game_window,' ');
        wattroff(game_window, COLOR_PAIR(4));
    } else {
        wattron(game_window, COLOR_PAIR(8));
        waddch(game_window,'m');
        waddch(game_window,'e');
        wattroff(game_window, COLOR_PAIR(8));
    }
    

    int num_act_players = (int) buf[25];
    int offset = 68;
    int humanx, humany;
    // start at one to skip the minotaur who has already been drawn
    int i;
    for (i = 1; i < num_act_players; i++) {
        humanx = buf[offset];
        humany = buf[offset + 1];

        wmove(game_window, humany, humanx); 

        // detect if player is current player, so they can be
        // marked with star to distinguish which human the player
        // is controlling
        if (strcmp(buf + offset - 20, player_name)) {
            wattron(game_window, COLOR_PAIR(5));
            waddch(game_window, ' ');
            waddch(game_window, ' ');
            wattroff(game_window, COLOR_PAIR(5));
        } else {
            wattron(game_window, COLOR_PAIR(7));
            waddch(game_window, 'm');
            waddch(game_window, 'e');
            wattroff(game_window, COLOR_PAIR(7));
        }

        offset = offset + 22;
    }

    wrefresh(game_window);
}


void parse_start_info(char *buf, char *player_name) {
    int i, offset, player_name_size;
    char curr_name[20];
    int num_active_players = buf[53];

    memcpy(map_name, buf + 21, 32);

    role = SPECTATOR;

    player_name_size = 20;
    
    for (i = 0; i < num_active_players; i++) {
        offset = 54 + (player_name_size * i);
        memcpy(curr_name, buf + offset, 20); 
        if (strcmp(curr_name, player_name) == 0) {
            if (i == 0) {
                role = MINOTAUR;    
            } else {
                role = HUMAN;
            }
        }
    }
}



int parse_reg_ack(char *buf) {
    // get the map-name from the ack
    int map_offset = 21;
    memcpy(map_name, buf + map_offset, 32);
    int name_valid_offset = map_offset + 32;
    return buf[name_valid_offset];
}


void ack_ping(ServerData *sd, char *player_name) {
    char ack[21];
    ack[0] = 8;
    memcpy(ack + 1, player_name, 20); 

    int n;
    n = sendto(sd->sockfd, ack, 21, 0, (struct sockaddr *) &(sd->serveraddr), sizeof(sd->serveraddr));
    if (n < 0) {
        client_exit(NULL);
        fprintf(stderr, "sendto error for ping ack\n");
        exit(1);
    }   
}

