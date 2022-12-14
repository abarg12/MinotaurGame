MinotaurGame
Multiplayer ascii-based game


--------------------------------
Server:
  - server.c
  - server.h
  - player.c
  - player.h
  - game_logic.c
  - game_logic.h

  To build the server, run `make` in the outermost
  directory (where server.c is located). 

  To run the server,
  `./a.out <port number> <round time> <file name> <required active players>`

--------------------------------
Client:
  - client/client.c
  - client/client.h

  To build the client, run `make` in the client
  directory.

  To run the client, `./a.out <server hostname> <port number>` 

