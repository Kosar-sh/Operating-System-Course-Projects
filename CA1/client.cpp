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
#include <signal.h>
#include <sstream>
#include <string>
#include <vector>
using namespace std;

int times_up = 0;
void alarm_handler(int sig)
{
    string time_up_sign="Times up,you didnt say your move\n";
    write(0,time_up_sign.c_str(),strlen(time_up_sign.c_str()));
    times_up = 1;
}

int connectServer(string IP, int port)
{
    int fd;
    struct sockaddr_in server_address;
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
        return -1;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    server_address.sin_addr.s_addr = inet_addr(IP.c_str());
    // server_address.sin_addr.s_addr = inet_addr("127.0.0.1");
    if (connect(fd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
    {
        close(fd);
        return -1;
    }
    return fd;
}

int connect_to_sub_server(int port, string IP)
{
    int fd;
    struct sockaddr_in server_address;
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
        return -1;
    server_address.sin_family = AF_INET;
    server_address.sin_port = port;
    server_address.sin_addr.s_addr = inet_addr(IP.c_str());
    if (connect(fd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
    {
        close(fd);
        return -1;
    }
    return fd;
}

int connect_to_broadcast(struct sockaddr_in &where, socklen_t &len)
{
    int sock, broadcast = 1, opt = 1;
    struct sockaddr_in bc_address;
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));
    setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
    bc_address.sin_family = AF_INET;
    bc_address.sin_port = htons(9090);
    bc_address.sin_addr.s_addr = inet_addr("127.255.255.255");
    int reu = bind(sock, (struct sockaddr *)&bc_address, sizeof(bc_address));
    where = bc_address;
    len = sizeof(bc_address);
    return sock;
}
int flag_username_enter = 0;
int available_rooms_clear = 0;

void collect_input_prameters(int argc, char const *argv[], string &IP, int &main_port)
{
    for (int i = 1; i < argc; i++)
    {
        if (argv[i] != "" && i == 1)
            IP = argv[i];
        else if (argv[i] != "" && i == 2)
            main_port = atoi(argv[i]);
    }
}

void add_to_read_fds(fd_set &readfds, int &fd, int &server_fd, int &maxfd, int &broad_cast_fd)
{
    FD_ZERO(&readfds);
    FD_SET(broad_cast_fd, &readfds);
    FD_SET(0, &readfds);
    FD_SET(fd, &readfds);
    if (server_fd > maxfd)
        maxfd = server_fd;
    else if (broad_cast_fd > maxfd)
        maxfd = broad_cast_fd;
    else if (fd > maxfd)
        maxfd = fd;
}

void choose_action(int fd)
{
    char buff[1024] = {};
    signal(SIGALRM, alarm_handler);
    siginterrupt(SIGALRM, 1);
    alarm(9);
    memset(buff, 0, sizeof(buff));
    int a = read(0, buff, sizeof(buff));
    alarm(0);
    if (a == -1)
    {
        send(fd, "0", 8, 0);
    }
    else
    {
        buff[strlen(buff) - 1] = '\0';
        send(fd, buff, strlen(buff), 0);
    }
}

bool attend_the_room(int &fd, string IP, int &sub_fd, string &token, istringstream &stream)
{
    getline(stream, token, '\n');
    int sub_port = stoi(token);
    sub_fd = connect_to_sub_server(sub_port, IP);
    if (sub_fd < 0)
    {
        return true;
    }
    write(0, " is connected now\n", 18);
    fd = sub_fd;
    return false;
}

bool handel_server_commands(char buff[1024], int &fd, string IP, int &sub_fd, int server_fd)
{
    if (strcmp(buff, "end") != 0)
    {
        write(0, "server respond:\n", 16);
        write(0, buff, strlen(buff));
    }
    string input;
    input.assign(buff);
    istringstream stream(input);
    string token;
    getline(stream, token, ' ');
    const char *port = token.c_str();
    if (strcmp(token.c_str(), "Choose") == 0)
        choose_action(fd);
    else if (strcmp(port, "port") == 0)
    {
        if (attend_the_room(fd, IP, sub_fd, token, stream))
            return true;
    }
    else if (strcmp(buff, "end") == 0)
    {
        close(fd);
        fd = server_fd;
    }
    return false;
}

bool handel_packets_fd(char buff[1024], int &fd, string IP, int &sub_fd, int server_fd, int valread)
{
    if (valread > 0)
    {
        if (handel_server_commands(buff, fd, IP, sub_fd, server_fd))
            return true;
    }
    else if (valread == 0)
    {
        write(0, "Connection closed by server\n", 28);
        return true;
    }
    return false;
}

void select_loop(fd_set &readfds, int &fd, int &server_fd, int &maxfd, struct sockaddr_in &broadcast_address,
                 int &broad_cast_fd, string IP, int &sub_fd, socklen_t &len_address)
{
    while (1)
    {
        char buff[1024] = {0};
        add_to_read_fds(readfds, fd, server_fd, maxfd, broad_cast_fd);
        select(maxfd + 1, &readfds, NULL, NULL, NULL);
        memset(buff, 0, sizeof(buff));
        if (FD_ISSET(fd, &readfds))
        {
            int valread = recv(fd, buff, sizeof(buff), 0);
            if (handel_packets_fd(buff, fd, IP, sub_fd, server_fd, valread))
                return;
        }
        else if (FD_ISSET(broad_cast_fd, &readfds))
        {
            int valread = recv(broad_cast_fd, buff, sizeof(buff), 0);
            if (valread > 0)
                write(0, buff, sizeof(buff));
        }
        else if (FD_ISSET(0, &readfds))
        {
            read(0, buff, sizeof(buff));
            send(fd, buff, strlen(buff), 0);
        }
    }
    return;
}

int main(int argc, char const *argv[])
{
    int fd, sub_fd, main_port, maxfd = 0;
    string IP;
    char buff[1024] = {0};
    fd_set readfds;
    struct sockaddr_in broadcast_address;
    collect_input_prameters(argc, argv, IP, main_port);
    fd = connectServer(IP, main_port);
    int server_fd = fd;
    if (fd < 0)
        return -1;
    socklen_t len_socket;
    int broad_cast_fd = connect_to_broadcast(broadcast_address, len_socket);
    select_loop(readfds, fd, server_fd, maxfd, broadcast_address, broad_cast_fd, IP, sub_fd, len_socket);
    close(fd);
    return 0;
}