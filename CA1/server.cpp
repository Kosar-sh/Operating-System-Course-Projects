#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <iostream>
#include <vector>
using namespace std;
#define MAX_CLIENTS 100


// port=8080
// IP=127.0.0.1

using namespace std;
struct record
{
    string name;
    int main_fd;
    int sub_server_fd;
    int fd_client;
    bool in_the_room;
    int win;
    int lose;
    int tie;
};
typedef record Record;

class Room
{
private:
    string name;
    int port;
    int subserverfd;
    int player1;
    int player2;
    bool semiful;
    bool full;
    string move1;
    string move2;
    int socket_broad;
    struct sockaddr_in broad_casting;

public:
    void check_for_finish(vector<Record> &client_record);
    const char *form_the_broadcast_message(vector<Record> &client_record,string &out);
    Room(string _name,int _port,int _fd,int _socket_broad,struct sockaddr_in broad_casting);
    bool no_move_on_sides( string move_player1,string &result);
    void both_move(string move_player1,string &result);
    string decide_who_wins();
    void start_the_game();
    int find_the_player(int id,int &which_player);
    int make_a_move(string move,int player);
    bool is_id_correct(int _id);
    bool is_the_room_semifull();
    bool is_the_room_full();
    int change_to_semifull(fd_set &master_set, char buffer[1024],int client_fd);
    int change_to_full(fd_set &master_set, char buffer[1024],int client_fd);
    int acceptClient(int server_fd);
    void free_the_room();
    string send_name_to_client(int receiver);
    string add_the_result_to_player_1(vector<Record> &client_record, string winner);
    string add_the_result_to_player_2(vector<Record> &client_record, string winner);
    string check_correctness_of_move(string _move);
    const char * form_the_responded_port(char buffer[1024]);
};

void Room::check_for_finish(vector<Record> &client_record)
{
    string out = "";
    if (this->move1 != "" && this->move2 != "")
    {
        out = decide_who_wins();
        const char *result = form_the_broadcast_message(client_record, out);
        int a = sendto(this->socket_broad, result, strlen(result) + 1, 0, (struct sockaddr *)&(this->broad_casting), sizeof(this->broad_casting));
        send(this->player1, "end", 4, 0);
        send(this->player2, "end", 4, 0);
        free_the_room();
    }    
}

const char *Room::form_the_broadcast_message(vector<Record> &client_record, string &out)
{
    string player1_name = add_the_result_to_player_1(client_record, out);
    string player2_name = add_the_result_to_player_2(client_record, out);
    if (out == "tie")
        out = "\nIn the room " + this->name + " the result is Tie between " + player1_name + "and " + player2_name;
    else if (out == "1")
        out = "\nIn the room " + this->name + " the winner is " + player1_name;
    else if (out == "2")
        out = "\nIn the room " + this->name + " the winner is " + player2_name;
    out = out + "\n";
    const char *result = out.c_str();
    return result;
}

Room::Room(string _name, int _port, int _fd, int _socket_broad, sockaddr_in _broad_casting)
{
    this->name = _name;
    this->port = _port;
    this->player1 = 0;
    this->player2 = 0;
    this->subserverfd = _fd;
    this->semiful = false;
    this->full = false;
    this->move1 = "";
    this->move2 = "";
    this->broad_casting = _broad_casting;
    this->socket_broad = _socket_broad;
}

string Room::decide_who_wins()
{
    string result = "tie";
    string move_player1 = this->move1;
    if (no_move_on_sides(move_player1, result) == false)
    {
        both_move(move_player1, result);
    }
    return result;
}

bool Room::no_move_on_sides(string move_player1, string &result)
{
    if ((move_player1.compare("0") == 0 && this->move2.compare("0") == 0) || (move_player1.compare("1") == 0 && this->move2.compare("1") == 0) || (move_player1.compare("2") == 0 && this->move2.compare("2") == 0) || (move_player1.compare("3") == 0 && this->move2.compare("3") == 0))
    {
        result = "tie";
        return true;
    }
    else if (move_player1.compare("0") == 0)
    {
        if (this->move2.compare("1") == 0 || this->move2.compare("2") == 0 || this->move2.compare("3") == 0)
        {
            result = "2";
            return true;
        }
    }
    else if (this->move2.compare("0") == 0)
    {
        if (move_player1.compare("1") == 0 || move_player1.compare("2") == 0 || move_player1.compare("3") == 0)
        {
            result = "1";
            return true;
        }
    }
    return false;
}

void Room::both_move(string move_player1, string &result)
{
    if (this->move2.compare("1") == 0)
    {
        if (move_player1.compare("3") == 0)
            result = "1";
        else if (move_player1.compare("2") == 0)
            result = "2";
    }
    else if (this->move2.compare("2") == 0)
    {
        if (move_player1.compare("3") == 0)
            result = "2";
        else if (move_player1.compare("1") == 0)
            result = "1";
    }
    else if (this->move2.compare("3") == 0)
    {
        if (move_player1.compare("2") == 0)
            result = "1";
        else if (move_player1.compare("1") == 0)
            result = "2";
    }
}

void Room::start_the_game()
{
    string respond = "Choose your move & Enter coresponding number:\n1-Paper\n2-Rock\n3-Scissors\n";
    const char *char_respond = respond.c_str();
    send(this->player1, char_respond, strlen(char_respond) + 1, 0);
    send(this->player2, char_respond, strlen(char_respond) + 1, 0);
}

int Room::find_the_player(int id, int &which_player)
{
    int found = -1;
    if (this->player1 == id)
    {
        found = 1;
        which_player = 1;
    }
    else if (this->player2 == id)
    {
        found = 1;
        which_player = 2;
    }
    return found;
}

int Room::make_a_move(string move, int which_player)
{
    if (which_player == 1 && this->full==true)
    {
        this->move1 = check_correctness_of_move(move);
    }
    else if(which_player == 2 && this->full==true)
    {
        this->move2 = check_correctness_of_move(move);
    }
    return 0;
}

bool Room::is_id_correct(int _id)
{
    if (this->port == _id)
    {
        return true;
    }

    return false;
}

bool Room::is_the_room_semifull()
{
    if (this->semiful == false)
    {
        return true;
    }
    return false;
}

bool Room::is_the_room_full()
{
    if (this->full == false)
    {
        return true;
    }

    return false;
}

int Room::acceptClient(int server_fd)
{
    int client_fd;
    struct sockaddr_in client_address;
    int address_len = sizeof(client_address);
    client_fd = accept(server_fd, (struct sockaddr *)&client_address, (socklen_t *)&address_len);
    return client_fd;
}

void Room::free_the_room()
{
    this->player1 = 0;
    this->player2 = 0;
    this->semiful = false;
    this->full = false;
    this->move1 = "";
    this->move2 = "";
}

string Room::send_name_to_client(int receiver)
{
    if (this->full == false)
    {
        return this->name;
    }
    return "";
}

string Room::add_the_result_to_player_1(vector<Record> &client_record, string winner)
{
    string name = "";
    for (int i = 0; i < client_record.size(); i++)
    {
        if (client_record[i].sub_server_fd == this->player1)
        {
            name = client_record[i].name;
            client_record[i].sub_server_fd = -1;
            if (winner == "tie")
                client_record[i].tie += 1;
            else if (winner == "1")
                client_record[i].win += 1;
            else if (winner == "2")
                client_record[i].lose += 1;
        }
    }
    return name;
}

string Room::add_the_result_to_player_2(vector<Record> &client_record, string winner)
{
    string name = "";
    for (int i = 0; i < client_record.size(); i++)
    {
        if (client_record[i].sub_server_fd == this->player2)
        {
            client_record[i].sub_server_fd = -1;
            name = client_record[i].name;
            if (winner == "tie")
                client_record[i].tie += 1;
            else if (winner == "2")
                client_record[i].win += 1;
            else if (winner == "1")
                client_record[i].lose += 1;
        }
    }
    return name;
}

string Room::check_correctness_of_move(string _move)
{
    if (_move == "0" || _move == "1" || _move == "2" || _move == "3")
    {
        return _move;
    }
    return "0";
}

int Room::change_to_semifull(fd_set &master_set, char buffer[1024], int client_fd)
{
    const char *result = form_the_responded_port(buffer);
    send(client_fd, result, strlen(result) + 1, 0);
    string port;
    string status="Client connect to room "+port.assign(buffer);
    write(0,status.c_str(),status.size());    
    int y = this->acceptClient(this->subserverfd);
    FD_SET(y, &master_set);
    this->semiful = true;
    this->player1 = y;
    return y;
}

int Room::change_to_full(fd_set &master_set, char buffer[1024], int client_fd)
{

    const char *result = form_the_responded_port(buffer);
    send(client_fd, result, strlen(result) + 1, 0);
    string port;
    string status="Client connect to room "+port.assign(buffer);
    write(0,status.c_str(),status.size());    
    int y = acceptClient(this->subserverfd);
    FD_SET(y, &master_set);
    this->full = true;
    this->player2 = y;
    return y;
}

const char *Room::form_the_responded_port(char buffer[1024])
{
    string s;
    s.assign(buffer);
    s[s.size() - 1] = '\0';
    string out = "port " + s;
    const char *result = out.c_str();
    return result;
}


int setupServer(int port, string IP)
{
    struct sockaddr_in address;
    int server_fd;
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(IP.c_str());
    // address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, 17);
    return server_fd;
}

int setup_sub_server(string IP, string &name, int &subserver_port)
{
    struct sockaddr_in address;
    int server_fd;
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(IP.c_str());
    address.sin_port = 0;
    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    struct sockaddr_in finall_add;
    socklen_t add_len = sizeof(finall_add);
    int port = getsockname(server_fd, (struct sockaddr *)&finall_add, &add_len);
    listen(server_fd, 5);
    name = to_string(finall_add.sin_port);
    subserver_port = finall_add.sin_port;
    return server_fd;
}

int acceptClient(int server_fd)
{
    int client_fd;
    struct sockaddr_in client_address;
    int address_len = sizeof(client_address);
    client_fd = accept(server_fd, (struct sockaddr *)&client_address, (socklen_t *)&address_len);
    return client_fd;
}

int find_the_player_room(vector<Room> &rooms, int id, int &which_player)
{
    int found = -1;
    for (int i = 0; i < rooms.size(); i++)
    {
        int flag = rooms[i].find_the_player(id, which_player);
        if (flag == 1)
        {
            found = i;
        }
    }
    return found;
}

int assign_the_moves(vector<Room> &rooms, string move, int player)
{
    int which_player = 0;
    int index = find_the_player_room(rooms, player, which_player);
    if (index != -1)
    {
        rooms[index].make_a_move(move, which_player);
    }
    return index;
}

int set_up_broadcast(struct sockaddr_in &mew)
{
    int sock, broadcast = 1, opt = 1;
    char buffer[1024] = {0};
    struct sockaddr_in bc_address;
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));
    setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
    bc_address.sin_family = AF_INET;
    bc_address.sin_port = htons(9090);
    bc_address.sin_addr.s_addr = inet_addr("127.255.255.255");

    bind(sock, (struct sockaddr *)&bc_address, sizeof(bc_address));
    mew = bc_address;
    return sock;
}

void add_new_client(int main_server_fd, fd_set &master_set, int &max_sd, vector<Record> &client_record)
{
    int new_socket = acceptClient(main_server_fd);
    FD_SET(new_socket, &master_set);
    if (new_socket > max_sd)
        max_sd = new_socket;
    send(new_socket, "Enter your name\n", 18, 0);
    client_record.push_back(Record());
    client_record[client_record.size() - 1].main_fd = new_socket;
    client_record[client_record.size() - 1].name = "";
    client_record[client_record.size() - 1].sub_server_fd = -1;
    client_record[client_record.size() - 1].in_the_room = false;
    client_record[client_record.size() - 1].win = 0;
    client_record[client_record.size() - 1].lose = 0;
    client_record[client_record.size() - 1].tie = 0;
}

void collect_input_prameters(int argc, char const *argv[], string &IP, int &main_port, int &count_room)
{
    for (int i = 1; i < argc; i++)
    {
        if (argv[i] != "" && i == 1)
            IP = argv[i];
        else if (argv[i] != "" && i == 2)
            main_port = atoi(argv[i]);
        else if (argv[i] != "" && i == 3)
            count_room = atoi(argv[i]);
    }
}

void make_the_subservers(vector<Room> &rooms, int count_room, vector<int> &subserver_fds,
                         int broad_cast_fd, struct sockaddr_in bc_address, string IP)
{
    for (int i = 0; i < count_room; i++)
    {
        string name;
        int port;
        int fd_sub = setup_sub_server(IP, name, port);
        subserver_fds.push_back(fd_sub);
        rooms.push_back(Room(name, port, fd_sub, broad_cast_fd, bc_address));
    }
}

void send_the_available_rooms(vector<Room> rooms, int client_fd)
{
    string all = "Available Rooms are as follow:\n";
    for (int i = 0; i < rooms.size(); i++)
    {
        string new_name = rooms[i].send_name_to_client(client_fd);
        all = all + new_name + "\n";
    }
    const char *here = all.c_str();
    send(client_fd, here, all.size(), 0);
}

void check_room_status(Room &room, char buffer[1024], fd_set &master_set, int client_fd,
                       Record &client_record, int &max_sd)
{
    if (room.is_the_room_semifull())
    {
        int y = room.change_to_semifull(master_set, buffer, client_fd);
        client_record.sub_server_fd = y;
        client_record.in_the_room = true;
        if (y > max_sd)
            max_sd = y;
    }
    else if (room.is_the_room_full())
    {
        int y = room.change_to_full(master_set, buffer, client_fd);
        client_record.sub_server_fd = y;
        client_record.in_the_room = true;
        if (y > max_sd)
            max_sd = y;
        room.start_the_game();
    }
    else
        send(client_fd, "This room is full choose another one\n", 41, 0);
}

void request_room_from_client(vector<Room> &rooms, char *buffer, fd_set &master_set, int client_fd,
                              Record &client_record, int &max_sd)
{
    bool exist = false;
    int choice = atoi(buffer);
    for (int j = 0; j < rooms.size(); j++)
    {
        if (rooms[j].is_id_correct(choice))
        {
            exist = true;
            check_room_status(rooms[j], buffer, master_set, client_fd, client_record, max_sd);
        }
    }
    if (exist == false)
    {
        send(client_fd, "Room doesnt exist\n", 18, 0);
    }
}

void print_all_records(vector<Record> client_record, int broad_cast_fd, struct sockaddr_in bc_address)
{
    string result = "";
    for (int i = 0; i < client_record.size(); i++)
    {
        result = result + client_record[i].name + "wins: " + to_string(client_record[i].win) + "\n";
    }
    const char *message = result.c_str();
    sendto(broad_cast_fd, message, strlen(message), 0, (struct sockaddr *)&(bc_address), sizeof(bc_address));
    sleep(5);
}

void finish_the_game(int broad_cast_fd, struct sockaddr_in bc_address,
                     vector<Record> client_record, int &process_run)
{
    char buffer[1024] = {0};
    memset(buffer, 0, sizeof(buffer));
    read(0, buffer, sizeof(buffer));
    if (strcmp(buffer, "end_game") == 0 || strcmp(buffer, "end_game\n") == 0)
    {
        sendto(broad_cast_fd, "Record\n", strlen("Record\n"), 0, (struct sockaddr *)&(bc_address), sizeof(bc_address));
        print_all_records(client_record, broad_cast_fd, bc_address);
        process_run = 0;
    }
}

void check_commmand_status(vector<Room> &rooms, Record &client_record,
                           int i, char buffer[1024], fd_set &master_set, int &max_sd)
{
    if (strcmp(buffer, "show available rooms\n") == 0)
        send_the_available_rooms(rooms, i);
    else if (client_record.name == "")
    {
        buffer[strlen(buffer) - 1] = ' ';
        client_record.name = buffer;
        string id = "New member: " + client_record.name + "\n";
        write(0, id.c_str(), strlen(id.c_str()));
        send_the_available_rooms(rooms, i);
    }
    else if (client_record.sub_server_fd == -1)
        request_room_from_client(rooms, buffer, master_set, i, client_record, max_sd);
}

void handel_client_request(vector<Room> &rooms, vector<Record> &client_record, char buffer[1024], int i, fd_set &master_set, int &max_sd)
{
    int bytes_received = recv(i, buffer, 1024, 0);
    if (bytes_received > 0)
    {
        for (int c = 0; c < client_record.size(); c++)
        {
            if (client_record[c].main_fd == i)
            {
                check_commmand_status(rooms, client_record[c], i, buffer, master_set, max_sd);
            }
            else if (client_record[c].sub_server_fd == i and client_record[c].in_the_room == true)
            {
                int index = assign_the_moves(rooms, buffer, i);
                rooms[index].check_for_finish(client_record);
            }
        }
    }
    else if (bytes_received == 0)
    {
        close(i);
        FD_CLR(i, &master_set);
    }
}

void find_max_file_descriptor(int &max_sd, int &broad_cast_fd, int &main_server_fd,
                              vector<int> &subserver_fds, fd_set &master_set)
{
    max_sd = main_server_fd;
    if (max_sd < broad_cast_fd)
    {
        max_sd = broad_cast_fd;
    }
    for (int i = 0; i < subserver_fds.size(); i++)
    {
        FD_SET(subserver_fds[i], &master_set);
        if (subserver_fds[i] > max_sd)
            max_sd = subserver_fds[i];
    }
    FD_SET(main_server_fd, &master_set);
    FD_SET(broad_cast_fd, &master_set);
    FD_SET(0, &master_set);
}

void catch_event_loop(fd_set &master_set, fd_set &working_set, int &max_sd, vector<Record> &client_record,
                      vector<Room> &rooms, struct sockaddr_in &bc_address, int &main_server_fd, int &broad_cast_fd)
{
    int process_run = 1;
    while (process_run)
    {
        char buffer[1024] = {0};
        working_set = master_set;
        int val = select(max_sd + 1, &working_set, NULL, NULL, NULL);
        if (val > 0)
        {
            for (int i = 0; i <= max_sd; i++)
            {
                if (FD_ISSET(i, &working_set))
                {
                    if (i == 0)
                        finish_the_game(broad_cast_fd, bc_address, client_record, process_run);
                    else if (i == main_server_fd)
                        add_new_client(main_server_fd, master_set, max_sd, client_record);
                    else if (i == broad_cast_fd)
                    {
                        int bytes_received = recv(i, buffer, 1024, 0);
                        if (bytes_received > 0)
                            write(0, buffer, sizeof(buffer));
                    }
                    else
                        handel_client_request(rooms, client_record, buffer, i, master_set, max_sd);
                }
            }
        }
    }
    return;
}

int main(int argc, char const *argv[])
{
    int main_server_fd, main_port, count_room, max_sd;
    fd_set master_set, working_set, write_set;
    char buffer[1024] = {0};
    string IP;
    collect_input_prameters(argc, argv, IP, main_port, count_room);
    main_server_fd = setupServer(main_port, IP);
    struct sockaddr_in bc_address;
    int broad_cast_fd = set_up_broadcast(bc_address);
    vector<int> subserver_fds;
    vector<Room> rooms;
    make_the_subservers(rooms, count_room, subserver_fds, broad_cast_fd, bc_address, IP);
    FD_ZERO(&master_set);
    find_max_file_descriptor(max_sd, broad_cast_fd, main_server_fd, subserver_fds, master_set);
    vector<Record> client_record;
    int process_run = 1;
    write(0, "The Server is ON\n", 18);
    catch_event_loop(master_set, working_set, max_sd, client_record, rooms, bc_address, main_server_fd, broad_cast_fd);
    close(main_server_fd);
    return 0;
}