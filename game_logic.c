#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <ncurses.h>
#include "game_logic.h"

// Map constants
#define MHEIGHT 28
#define MWIDTH  96

void load_map(char *file_name, char *map) {
    FILE *fptr = fopen(file_name, "rb");
    if (fptr == NULL) {
        fprintf(stderr, "Map does not exist at specified location\n");
        exit(1);
    }

    map = malloc(MWIDTH * MHEIGHT);
    int n = fread(map, 1, MWIDTH * MHEIGHT, fptr);
    if (n != (MWIDTH * MHEIGHT)) {
        fprintf(stderr, "Map is not of right size, or was read improperly\n");
        exit(1); 
    }

    fclose(fptr);
}
