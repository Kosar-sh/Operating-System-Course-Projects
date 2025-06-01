# Operating-System-Course-Projects

## CA1: Rock, Paper, Scissors Game
This project is a client-server implementation of the classic "Rock, Paper, Scissors" game. The primary goal is to practice with system calls, specifically focusing on socket programming (IPv4) to manage multiple game rooms and players concurrently.

### Features
Game Rooms: The server hosts multiple rooms where two players can face off in a game.

Timed Moves: Players have 10 seconds to secretly make their choice (Rock, Paper, or Scissors). A player who fails to choose in time forfeits the round.

Broadcast Results: Room winners are announced to everyone via broadcast. The organizer also broadcasts the final scores when the game ends.

Centralized Control: The game organizer can end the entire session at any time.

### How to Compile and Run
1. Compilation

  A Makefile is provided. To compile the source code, simply run the make command in the project's root directory:

  `make`

2. Running the Server (Game Organizer)

  Use the following command to start the server:

  `./server.out {IP} {Port} {#Rooms}`

  - {IP}: The IP address the organizer will run on.
  - {Port}: The port number for the organizer to listen on.
  - {#Rooms}: The number of game rooms to create.

3. Running the Client (Player)

  To launch a player client, use this command:

  `./client.out {IP} {Port}`

  - {IP}: The IP address of the game organizer.
  - {Port}: The port number of the game organizer.