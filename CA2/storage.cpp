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
Logger lg("storage");

using namespace std;
namespace fs = std::filesystem;

typedef struct
{
    string filePath;
    string file_name;
    unordered_map<string, double> products_wanted; // store the profit
} IPC_Record;

struct ProductInfo
{
    string name;
    double how_much;
    int how_many;
    string status;
};

int total_profit = 0;

void assign_values_to_record(char buffer[1024], ssize_t bytesRead, IPC_Record &records)
{
    buffer[bytesRead] = '\0';
    stringstream ss(buffer);
    string genre, number, file_name;
    getline(ss, genre, '\n');
    getline(ss, file_name, '\n');
    records.filePath = genre;
    records.file_name = file_name;
    getline(ss, number, '\n');
    for (int i = 0; i < stoi(number); i++)
    {
        string pro;
        getline(ss, pro, '\n');
        records.products_wanted[pro] = 0;
    }
}

IPC_Record get_unnamed_pipe_data(int pipe_fd)
{
    IPC_Record records;
    string input;
    char buffer[1024];
    ssize_t bytesRead = read(pipe_fd, buffer, sizeof(buffer) - 1);
    if (bytesRead > 0)
    {
        assign_values_to_record(buffer, bytesRead, records);
    }
    close(pipe_fd);
    return records;
}

void add_new_product(vector<ProductInfo> &pro_saved, const string &product, const string &price, const string &quantity)
{
    pro_saved.push_back(ProductInfo{});
    pro_saved.back().name = product;
    pro_saved.back().how_much = stod(price);
    pro_saved.back().how_many = atoi(quantity.c_str());
    pro_saved.back().status = "input";
}

void less_than_wanted(const string &product, const string &price,
                      ProductInfo &pro_saved, IPC_Record &records, int &quantity_int)
{
    double difference = (stod(price) - pro_saved.how_much) * pro_saved.how_many;
    total_profit = total_profit + difference;
    records.products_wanted[product] = records.products_wanted[product] + difference;
    quantity_int = quantity_int - pro_saved.how_many;
    pro_saved.how_many = 0;
}

void more_than_wanted(const string &product, const string &price,
                      ProductInfo &pro_saved, IPC_Record &records, int &quantity_int)
{
    pro_saved.how_many = pro_saved.how_many - quantity_int;
    double difference = (stod(price) - pro_saved.how_much) * quantity_int;
    total_profit = total_profit + difference;
    records.products_wanted[product] = records.products_wanted[product] + difference;
}

void sell_new_product(vector<ProductInfo> &pro_saved, const string &product,
                      const string &price, const string &quantity, IPC_Record &records)
{
    int quantity_int = stoi(quantity);
    for (int i = 0; i < pro_saved.size(); i++)
    {
        if (pro_saved[i].name == product && pro_saved[i].how_many != 0)
        {
            if (pro_saved[i].how_many >= quantity_int)
            {
                more_than_wanted(product, price, pro_saved[i], records, quantity_int);
                break;
            }
            else
            {
                less_than_wanted(product, price, pro_saved[i], records, quantity_int);
            }
        }
    }
}

bool have_the_product(const string &product, IPC_Record &records)
{
    for (const auto &fifoFile : records.products_wanted)
    {
        if (fifoFile.first == product)
        {
            return true;
        }
    }
    return false;
}

void handel_the_products(vector<ProductInfo> &pro_saved, const string &product,
                         const string &price, const string &quantity, const string &status, IPC_Record &records)
{
    if (!have_the_product(product, records))
        return;
    if (status == "input")
    {
        add_new_product(pro_saved, product, price, quantity);
    }
    else
    {
        sell_new_product(pro_saved, product, price, quantity, records);
    }
}

vector<string> read_csv(const string &line)
{
    vector<string> values;
    stringstream ss(line);
    string value;
    while (getline(ss, value, ','))
    {
        size_t start = value.find_first_not_of(" \t\n\r");
        size_t end = value.find_last_not_of(" \t\n\r");
        values.push_back(start == string::npos ? "" : value.substr(start, end - start + 1));
    }
    return values;
}

void read_line_data(string &line, IPC_Record &records, vector<ProductInfo> &pro_saved)
{
    if (line.empty())
    {
        return;
    }
    vector<string> fields = read_csv(line);
    if (fields.size() == 4)
    {
        handel_the_products(pro_saved, fields[0], fields[1], fields[2], fields[3], records);
    }
    else
    {
        lg.error("Unformed line skipped: " + line);
    }
}

bool process_the_file(IPC_Record &records, vector<ProductInfo> &pro_saved)
{
    ifstream file(records.filePath);
    if (!file.is_open())
    {
        lg.error("Could not open file: " + records.filePath);
        return false;
    }
    string line;
    while (getline(file, line))
    {
        read_line_data(line, records, pro_saved);
    }
    return true;
}

void extract_file_name(IPC_Record &records, string filename)
{
    size_t last_dot = filename.find('.');

    if (last_dot != string::npos)
    {
        filename = filename.substr(0, last_dot);
    }
    size_t last_slash = filename.find_last_of('"');
    if (last_dot != string::npos)
    {
        filename = filename.substr(last_slash + 1);
    }
    lg.update_name(filename);
    records.file_name = filename;
}
void find_the_remainder(string product, vector<ProductInfo> &pro_saved, string &count, string &price)
{
    int _price = 0;
    int _count = 0;
    for (int i = 0; i < pro_saved.size(); i++)
    {
        if (pro_saved[i].name == product && pro_saved[i].how_many != 0)
        {
            _price = _price + (pro_saved[i].how_much * pro_saved[i].how_many);
            _count = _count + pro_saved[i].how_many;
        }
    }
    count = to_string(_count);
    price = to_string(_price);
}

bool send_data_to_parts(string pro_name, vector<ProductInfo> &pro_saved, int write_fd, string fifo_name)
{
    string count, price;
    find_the_remainder(pro_name, pro_saved, count, price);
    string final_str = count + "\n" + price + '\n';
    if (write(write_fd, final_str.c_str(), final_str.size()) == -1)
    {
        return false;
    }
    close(write_fd);
    lg.info("Data Sent To Pipe " + fifo_name);
    return true;
}

bool prepare_data_to_parts(IPC_Record &records, vector<ProductInfo> &pro_saved)
{
    for (const auto &genre : records.products_wanted)
    {
        string fifo_name = genre.first + "-" + records.file_name;
        int fd = open(fifo_name.c_str(), O_WRONLY);
        if (fd == -1)
        {
            lg.error("open");
            return false;
        }
        if (!send_data_to_parts(genre.first, pro_saved, fd, fifo_name))
        {
            lg.error("write");
        }
    }
    return true;
}

bool send_data_to_main(IPC_Record &records, int fd_write)
{
    ostringstream ss;
    string data_pass = to_string(records.products_wanted.size()) + '\n';
    for (const auto &fifoFile : records.products_wanted)
    {
        data_pass = data_pass + fifoFile.first + '\n' + to_string(fifoFile.second) + '\n';
    }
    if (write(fd_write, data_pass.c_str(), data_pass.size()) == -1)
    {
        lg.error("Send records to child failed");
        return false;
    }
    close(fd_write);
    return true;
}

int main(int argc, const char *argv[])
{
    int fd_read = stoi(argv[1]);
    int fd_write = stoi(argv[2]);
    IPC_Record records = get_unnamed_pipe_data(fd_read);
    extract_file_name(records, records.file_name);
    vector<ProductInfo> pro_saved;
    lg.info("Warehouse Started");
    if (process_the_file(records, pro_saved))
        lg.info("File Processed Successfully.");
    else
        lg.error("Failed to process the file.");
    if (send_data_to_main(records, fd_write))
        lg.info("Successfully Send Profit To Main.");
    if (!prepare_data_to_parts(records, pro_saved))
        return EXIT_FAILURE;
    lg.info("Finish Task In Warehouse");
    return 0;
}