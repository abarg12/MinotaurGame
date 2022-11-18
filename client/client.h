/*** client.h ***/

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <ncurses.h>
#include <sys/types.h>
#include <netdb.h> 

typedef enum PlayerState {
    IN_LOBBY,
    PLAYING,
    SPECTATING,
    END_OF_GAME,
} PlayerState;

typedef struct ServerData {
    char *hostname;
    int port_num;
    int sockfd;
    int serverlen;
    struct sockaddr_in serveraddr;
    struct hostent *server;
} ServerData;

typedef struct __attribute__((__packed__)) InstrStruct {
    char type;
    char ID[20];
    char DATA[512];
} InstrStruct;

