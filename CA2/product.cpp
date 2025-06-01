#include <stdio.h>
#include <iostream>
#include <filesystem>
#include <vector>
#include <string>
#include <fstream>
#include <regex>
#include <sstream>
#include <unordered_map>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "logger.hpp"

Logger lg("product");
using namespace std;
typedef struct
{
    string product_name;
    unordered_map<string, int> fifo_info;
} IPC_Record;

struct Record
{
    long count;
    long price;
};

IPC_Record get_unnamed_pipe_data(int pipe_fd)
{
    IPC_Record data;
    char buffer[1024];
    ssize_t bytesRead = read(pipe_fd, buffer, sizeof(buffer) - 1);
    if (bytesRead > 0)
    {
        buffer[bytesRead] = '\0';
        stringstream ss(buffer);
        string product_name, number;
        getline(ss, product_name, '\n');
        data.product_name = product_name;
        getline(ss, number, '\n');
        for (int i = 0; i < stoi(number); i++)
        {
            string pro;
            getline(ss, pro, '\n');
            data.fifo_info[pro] = -1;
        }
    }
    close(pipe_fd);
    return data;
}

void read_input_fifo(int fd, Record &records, string pipe)
{
    char buffer[1024] = {0};
    int count = read(fd, buffer, 1024);
    if (count == -1)
    {
        lg.error("read");
        return;
    }
    buffer[count] = '\0';
    stringstream ss(buffer);
    string quantity, price;
    getline(ss, quantity, '\n');
    getline(ss, price, '\n');
    records.count = records.count + stod(quantity);
    records.price = records.price + stod(price);
    lg.info("The Part Get The Record From " + pipe);
    return;
}

void find_fd_trigger(IPC_Record &data, Record &records,
                     fd_set &master_set, fd_set &working_set, int &wait_files, int &ready)
{
    for (const auto &fifoFile : data.fifo_info)
    {
        if (FD_ISSET(fifoFile.second, &working_set))
        {
            read_input_fifo(fifoFile.second, records, fifoFile.first);
            close(fifoFile.second);
            FD_CLR(fifoFile.second, &master_set);
            --wait_files;
            if (--ready == 0)
                break;
        }
    }
}
int get_warehouse_messages(IPC_Record &data, Record &records)
{
    fd_set master_set, working_set;
    FD_ZERO(&master_set);
    for (const auto &fifoFile : data.fifo_info)
        FD_SET(fifoFile.second, &master_set);
    int wait_files = data.fifo_info.size();
    while (wait_files)
    {
        working_set = master_set;
        int ready = select(FD_SETSIZE, &working_set, NULL, NULL, NULL);
        if (ready == -1)
        {
            lg.error("Select Failed");
            return -1;
        }
        find_fd_trigger(data, records, master_set, working_set, wait_files, ready);
    }
    return 0;
}

bool get_fifo_fd(IPC_Record &data)
{
    for (auto &fifoFile : data.fifo_info)
    {
        fifoFile.second = open(fifoFile.first.c_str(), O_RDONLY | O_NONBLOCK);
        if (fifoFile.second == -1)
        {
            lg.error("open");
            return false;
        }
    }
    lg.info("All Named Pipes Opened Successfully");
    return true;
}

bool send_leftovers_to_parent(IPC_Record &data, Record &records, int fd_write)
{
    string data_pass = data.product_name + "\n" + to_string(records.count) + "\n" + to_string(records.price) + "\n";
    if (write(fd_write, data_pass.c_str(), data_pass.size()) == -1)
    {
        lg.error("Send data to child failed");
        return false;
    }
    close(fd_write);
    return true;
}

int main(int argc, const char *argv[])
{
    int fd_read = stoi(argv[1]);
    int fd_write = stoi(argv[2]);
    IPC_Record data = get_unnamed_pipe_data(fd_read);
    Record records;
    records.count = 0;
    records.price = 0;
    lg.update_name(data.product_name);
    lg.info("Product-Reduce start Work");
    if (!get_fifo_fd(data))
        return EXIT_FAILURE;
    if (get_warehouse_messages(data, records) == -1)
        return EXIT_FAILURE;
    if (!send_leftovers_to_parent(data, records, fd_write))
        return EXIT_FAILURE;
    return 0;
}