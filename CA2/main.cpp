#include <stdio.h>
#include <iostream>
#include <filesystem>
#include <vector>
#include <string>
#include <fstream>
#include <regex>
#include <sstream>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "logger.hpp"

Logger lg("main");
using namespace std;
namespace fs = std::filesystem;

int total_profit = 0;

struct StorageProfit
{
    string product;
    double total;
};
struct LeftOver
{
    string name;
    int count;
    int price;
};

void find_csv_stores(string directory_name, vector<string> &csv_file_name,
                     vector<fs::directory_entry> &libraries)
{
    for (const auto &entry : fs::directory_iterator(directory_name))
    {
        if (entry.is_regular_file())
        {
            string fileName = entry.path().filename().string();
            if (entry.path().extension() == ".csv" && fileName.find("Parts") == string::npos)
            {
                libraries.push_back(entry);
                csv_file_name.push_back(fileName);
            }
        }
    }
}

vector<string> split(const string &str, char delim)
{
    vector<string> elements;
    istringstream ss(str);
    string item;
    while (getline(ss, item, delim))
    {
        item.erase(0, item.find_first_not_of(' '));
        item.erase(item.find_last_not_of(' ') + 1);
        if (!item.empty())
            elements.push_back(item);
    }
    return elements;
}

bool get_products(const string &path, vector<string> &products)
{
    string genrePath = path + "/" + "Parts.csv";
    ifstream file(genrePath);
    if (!file.is_open())
    {
        lg.error("It failed to open the Parts.csv");
        return false;
    }
    string line;
    getline(file, line);
    products = split(line, ',');
    if (products.empty())
    {
        lg.error("No product was found");
        return false;
    }
    return true;
}

pid_t fork_the_stores(int pipefd[2], int pipefd2[2])
{
    pid_t pid = fork();
    if (pid == -1)
    {
        lg.error("Fork failed");
    }
    if (pid == 0)
    {
        close(pipefd[1]);
        close(pipefd2[0]);
        char pipefd1_str[32];
        char pipefd2_str[32];
        snprintf(pipefd1_str, sizeof(pipefd1_str), "%d", pipefd[0]);
        snprintf(pipefd2_str, sizeof(pipefd2_str), "%d", pipefd2[1]);
        execl("./storage", "storage", pipefd1_str, pipefd2_str, nullptr);
        lg.error("Execl Failed");
        exit(1);
    }
    close(pipefd[0]);
    close(pipefd2[1]);
    return pid;
}

bool send_data_to_warehouses(const fs::directory_entry &lib, const vector<string> &products, int fd)
{
    ostringstream ss;
    ss << lib.path().string() << endl;
    ss << lib.path().filename() << endl;
    ss << products.size() << endl;
    for (const auto &product : products)
        ss << product << endl;
    string data = ss.str();
    if (write(fd, data.c_str(), data.size()) == -1)
    {
        lg.error("Send data to child failed");
        return false;
    }
    return true;
}

void assign_particular_profit(vector<StorageProfit> &storage_profits, stringstream &ss)
{
    string name, profit;
    getline(ss, name, '\n');
    getline(ss, profit, '\n');
    for (int i = 0; i < storage_profits.size(); i++)
    {
        if (storage_profits[i].product == name)
        {
            storage_profits[i].total = storage_profits[i].total + stod(profit);
        }
    }
}

void read_storage_results(vector<StorageProfit> &storage_profits, int read_fd)
{
    char buffer[1024];
    ssize_t bytesRead = read(read_fd, buffer, sizeof(buffer) - 1);
    if (bytesRead > 0)
    {
        buffer[bytesRead] = '\0';
        stringstream ss(buffer);
        string number_product;
        getline(ss, number_product, '\n');
        for (int i = 0; i < stoi(number_product); i++)
        {
            assign_particular_profit(storage_profits, ss);
        }
    }
    close(read_fd);
}

bool make_warehouse_processes(const vector<fs::directory_entry> &stores, const vector<string> &products,
                              vector<pid_t> &pids, vector<int> &read)
{
    for (const auto &store : stores)
    {
        int pipefd[2];
        int pipefd2[2];
        if (pipe(pipefd) == -1 || pipe(pipefd2) == -1)
        {
            lg.error("pipe wasn't build");
            return false;
        }
        pid_t pid = fork_the_stores(pipefd, pipefd2);
        if (pid == -1)
            return false;
        pids.push_back(pid);
        if (!send_data_to_warehouses(store, products, pipefd[1]))
            return false;
        if (close(pipefd[1]) == -1)
            lg.error("closing the pipe failed");
        read.push_back(pipefd2[0]);
    }
    return true;
}

pid_t fork_the_products(int pipefd[2], int pipefd2[2])
{
    pid_t pid = fork();
    if (pid == -1)
    {
        lg.error("Fork failed");
    }
    if (pid == 0)
    {
        close(pipefd[1]);
        close(pipefd2[0]);
        char pipefd1_str[32];
        char pipefd2_str[32];
        snprintf(pipefd1_str, sizeof(pipefd1_str), "%d", pipefd[0]);
        snprintf(pipefd2_str, sizeof(pipefd2_str), "%d", pipefd2[1]);
        execl("./product", "product", pipefd1_str, pipefd2_str, nullptr);
        lg.error("Execl Failed");
        exit(1);
    }
    close(pipefd[0]);
    close(pipefd2[1]);
    return pid;
}

bool send_to_parts(const vector<string> &fifoFiles, const string &genre, int fd)
{
    ostringstream ss;
    ss << genre << endl;
    ss << fifoFiles.size() << endl;
    for (const auto &fifo : fifoFiles)
        ss << fifo << endl;
    string data = ss.str();
    if (write(fd, data.c_str(), data.size()) == -1)
    {
        lg.error("Write for parts failed");
        return false;
    }
    return true;
}

bool make_parts_processes(const vector<vector<string>> &fifoFiles,
                          const vector<string> &parts, vector<pid_t> &pids, vector<int> &pipe_read)
{
    for (size_t i = 0; i < parts.size(); i++)
    {
        int pipefd[2];
        int pipefd2[2];
        if (pipe(pipefd) == -1 || pipe(pipefd2) == -1)
        {
            lg.error("create pipe failed");
            return false;
        }
        pid_t pid = fork_the_products(pipefd, pipefd2);
        if (pid == -1)
            return false;
        pids.push_back(pid);
        if (!send_to_parts(fifoFiles[i], parts[i], pipefd[1]))
            return false;
        if (close(pipefd[1]) == -1)
            lg.error("close pipe failed");
        pipe_read.push_back(pipefd2[0]);
    }
    return true;
}

string extract_file_name(string filename)
{
    size_t last_dot = filename.find('.');
    if (last_dot != string::npos)
    {
        filename = filename.substr(0, last_dot);
    }
    size_t last_slash = filename.find_last_of('/');
    if (last_dot != string::npos)
    {
        filename = filename.substr(last_slash + 1);
    }
    return filename;
}

bool make_named_pipes(const vector<string> &products,
                      const vector<fs::directory_entry> &stores, vector<vector<string>> &fifoFiles)
{
    for (const auto &product : products)
    {
        vector<string> fifoFilesForProducts;
        for (const auto &store : stores)
        {
            string name = extract_file_name(store.path().filename());
            string fifoFile = product + "-" + (name);
            if (mkfifo(fifoFile.c_str(), 0666) == -1)
            {
                lg.error("Make fifo failed");
                return false;
            }
            fifoFilesForProducts.push_back(fifoFile);
        }
        lg.info_file(product);
        fifoFiles.push_back(fifoFilesForProducts);
    }
    return true;
}

void wait_all_to_exit(const vector<pid_t> &pids)
{
    for (const auto &pid : pids)
    {
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status))
        {
            if (WEXITSTATUS(status) != EXIT_SUCCESS)
                lg.error("Worker with pid " + to_string(pid) + " exited with status " + to_string(status));
        }
    }
    lg.main_msg("All Children Exit with status 0 ");
}

void remove_named_pipes(const vector<vector<string>> &fifoFiles)
{
    for (const auto &fifoFilesForGenre : fifoFiles)
        for (const auto &fifoFile : fifoFilesForGenre)
            if (unlink(fifoFile.c_str()) == -1)
                lg.error("unlink");
}

void extract_value_from_pipe(vector<LeftOver> &leftovers, char buffer[1024])
{
    stringstream ss(buffer);
    string quantity, price, product;
    getline(ss, product, '\n');
    getline(ss, quantity, '\n');
    getline(ss, price, '\n');
    for (int i = 0; i < leftovers.size(); i++)
    {
        if (leftovers[i].name == product)
        {
            leftovers[i].count = stod(quantity);
            leftovers[i].price = stod(price);
        }
    }
}

void read_input(int fd, vector<LeftOver> &leftovers)
{
    char buffer[1024] = {0};
    int count = read(fd, buffer, 1024);
    if (count == -1)
    {
        lg.error("Read from pipe failed");
        return;
    }
    buffer[count] = '\0';
    extract_value_from_pipe(leftovers, buffer);
    return;
}

void find_fd_trigger(vector<int> pipe_res, vector<LeftOver> &leftovers,
                     fd_set &master_set, fd_set &working_set, int &wait_files, int &ready)
{
    for (const auto &fifoFile : pipe_res)
    {
        if (FD_ISSET(fifoFile, &working_set))
        {
            read_input(fifoFile, leftovers);
            close(fifoFile);
            FD_CLR(fifoFile, &master_set);
            --wait_files;
            if (--ready == 0)
                break;
        }
    }
}

bool wait_for_parts_respond(vector<int> pipe_res, vector<LeftOver> &leftovers)
{
    long long count = 0, price = 0;
    fd_set master_set, working_set;
    FD_ZERO(&master_set);
    for (const auto &read_end : pipe_res)
        FD_SET(read_end, &master_set);
    int wait_files = pipe_res.size();
    while (wait_files)
    {
        working_set = master_set;
        int ready = select(FD_SETSIZE, &working_set, NULL, NULL, NULL);
        if (ready == -1)
        {
            lg.error("Select failed");
            return false;
        }
        find_fd_trigger(pipe_res, leftovers, master_set, working_set, wait_files, ready);
    }
    return true;
}

void read_from_warehouse(vector<StorageProfit> &storage_profits, int read_fd)
{
    char buffer[1024];
    ssize_t bytesRead = read(read_fd, buffer, sizeof(buffer) - 1);
    if (bytesRead > 0)
    {
        buffer[bytesRead] = '\0';
        stringstream ss(buffer);
        string number_product;
        getline(ss, number_product, '\n');
        for (int i = 0; i < stoi(number_product); i++)
        {
            assign_particular_profit(storage_profits, ss);
        }
    }
    close(read_fd);
}

void which_warehouse_trigger(vector<int> pipe_res, vector<StorageProfit> &storage_profits,
                             fd_set &master_set, fd_set &working_set, int &wait_files, int &ready)
{
    for (const auto &read_fd : pipe_res)
    {
        if (FD_ISSET(read_fd, &working_set))
        {
            read_from_warehouse(storage_profits, read_fd);
            close(read_fd);
            FD_CLR(read_fd, &master_set);
            --wait_files;
            if (--ready == 0)
                break;
        }
    }
}

bool wait_for_warehouse_respond(vector<int> pipe_res, vector<StorageProfit> &storage_profits)
{
    long long count = 0, price = 0;
    fd_set master_set, working_set;
    FD_ZERO(&master_set);
    for (const auto &read_end : pipe_res)
        FD_SET(read_end, &master_set);
    int wait_files = pipe_res.size();
    while (wait_files)
    {
        working_set = master_set;
        int ready = select(FD_SETSIZE, &working_set, NULL, NULL, NULL);
        if (ready == -1)
        {
            lg.error("Select failed");
            return false;
        }
        which_warehouse_trigger(pipe_res, storage_profits, master_set, working_set, wait_files, ready);
    }
    return true;
}

bool make_the_children(const vector<fs::directory_entry> &stores, const vector<string> &products,
                       vector<vector<string>> &fifoFiles, vector<pid_t> &pids, vector<int> &parts_read_fd,
                       vector<int> &stores_read_fd)
{
    if (!make_named_pipes(products, stores, fifoFiles))
        return false;
    if (!make_warehouse_processes(stores, products, pids, stores_read_fd))
        return false;
    else
        lg.main_msg("We built Children For Warehouses");
    if (!make_parts_processes(fifoFiles, products, pids, parts_read_fd))
        return false;
    else
        lg.main_msg("We built Children For Products");
    return true;
}

bool wait_for_response(vector<StorageProfit> &storage_profits, vector<int> &parts_read_fd,
                       vector<int> &stores_read_fd, vector<LeftOver> &leftovers,vector<pid_t> &pids)
{
    if (!wait_for_warehouse_respond(stores_read_fd, storage_profits))
        return false;
    if (!wait_for_parts_respond(parts_read_fd, leftovers))
        return false;
    wait_all_to_exit(pids);
    return true;
}

bool do_calculation_on_warehouses(const vector<fs::directory_entry> &stores, const vector<string> &products,
                                  vector<StorageProfit> &storage_profits, vector<LeftOver> &leftovers)
{
    vector<pid_t> pids;
    vector<int> parts_read_fd, stores_read_fd;
    vector<vector<string>> fifoFiles;
    if (!make_the_children(stores, products, fifoFiles, pids, parts_read_fd, stores_read_fd))
        return false;
    if (!wait_for_response(storage_profits, parts_read_fd, stores_read_fd, leftovers,pids))
        return false;
    lg.main_msg("ALL Responses Was Collected");
    remove_named_pipes(fifoFiles);
    return true;
}

vector<string> extract_products(string choosen, vector<string> products)
{
    vector<string> choose_ones;
    stringstream ss(choosen);
    string number_product;
    while (getline(ss, number_product, ' '))
    {
        int number_p = stoi(number_product);
        if (number_p <= products.size() && number_p != 0)
        {
            choose_ones.push_back(products[number_p - 1]);
        }
    }
    return choose_ones;
}

void make_products_records(vector<string> &choosen_products, vector<LeftOver> &leftovers,
                           vector<StorageProfit> &storage_profits, string choosen, vector<string> products)
{
    choosen_products = extract_products(choosen, products);
    for (int i = 0; i < choosen_products.size(); i++)
    {
        storage_profits.push_back(StorageProfit{});
        storage_profits[storage_profits.size() - 1].product = choosen_products[i];
        storage_profits[storage_profits.size() - 1].total = 0;
        leftovers.push_back(LeftOver{});
        leftovers[leftovers.size() - 1].name = choosen_products[i];
        leftovers[leftovers.size() - 1].count = 0;
        leftovers[leftovers.size() - 1].price = 0;
    }
}

void print_record(vector<LeftOver> &leftovers, vector<StorageProfit> &storage_profits)
{
    lg.main_msg("The Report Of Leftovers:");
    for (int i = 0; i < leftovers.size(); i++)
    {
        lg.print_leftovers(leftovers[i].name, leftovers[i].count, leftovers[i].price);
    }
    for (int i = 0; i < storage_profits.size(); i++)
    {
        total_profit = total_profit + storage_profits[i].total;
    }
    lg.print_total_profit(total_profit);
}

void start_process(vector<string> &csv_file_name, vector<fs::directory_entry> &stores, vector<string> &products,
                   vector<StorageProfit> &storage_profits, string &directory_name, vector<pid_t> &pids,
                   string &choosen, vector<string> &choosen_products, vector<LeftOver> &leftovers)
{
    try
    {
        find_csv_stores(directory_name, csv_file_name, stores);
        get_products(directory_name, products);
        lg.start_message(products);
        getline(cin, choosen, '\n');
        make_products_records(choosen_products, leftovers, storage_profits, choosen, products);
        do_calculation_on_warehouses(stores, choosen_products, storage_profits, leftovers);
        print_record(leftovers, storage_profits);
    }
    catch (const exception &e)
    {
        cerr << "Error: " << e.what() << '\n';
        lg.error(e.what());
    }
}

string ask_for_file_path()
{
    lg.warning("Please Enter The Path To Store Folder");
    string path;
    getline(cin, path, '\n');
    return path;
}

int main(int argc, const char *argv[])
{
    string directory_name;
    if (argc != 2)
        directory_name = ask_for_file_path();
    else
        directory_name = argv[1];
    vector<string> csv_file_name;
    vector<fs::directory_entry> stores;
    vector<string> products;
    vector<StorageProfit> storage_profits;
    vector<pid_t> pids;
    string choosen;
    vector<string> choosen_products;
    vector<LeftOver> leftovers;
    start_process(csv_file_name, stores, products, storage_profits, directory_name,
                  pids, choosen, choosen_products, leftovers);
    return 0;
}