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

## CA2: Warehouse Inventory Management with MapReduce

This project simulates a warehouse inventory and profit calculation system for a food distribution company using the MapReduce programming model. The goal is to process data from multiple warehouses concurrently by managing child processes and their communication channels.  The program calculates the total inventory for each product and the profit generated from sales across all locations. 

### How It Works
This program uses a MapReduce model to process inventory data in parallel.

Coordinator: A main process starts by creating two types of child processes: one "warehouse process" for each city's data file and one "item process" for each unique product.


Map Phase (Warehouse Processes): Each warehouse process reads its assigned CSV file to calculate the remaining inventory for each product and the total profit from sales (using FIFO logic). This inventory data is then sent to the appropriate item processes.


Reduce Phase (Item Processes): Each item process receives data from all warehouses for a single product. It aggregates this information to find the total stock and value for that specific item across the entire company.

Final Report: All calculated results are sent back to the main process. The program then interactively asks the user which products to include in the final report before printing the output and terminating


### Inter-Process Communication
Communication between processes is handled exclusively using pipes. 

Unnamed Pipes: Used for communication between a parent and its direct children (e.g., main process to warehouse/item processes). 

Named Pipes (FIFOs): Used for communication between processes that are not directly related, such as a warehouse process sending data to an item process. 

### How to Compile and Run
The project must be compiled and run on a Linux operating system using the provided Makefile. 

1. Compile the Program:
    Open a terminal in the project's root directory and run the make command. This will compile separate executables for each process type (main, warehouse, item). 

    `make`

2. Run the Program:
    Execute the main program and provide the path to the directory containing the warehouse data files.

    `./main.out <path_to_stores_directory>`