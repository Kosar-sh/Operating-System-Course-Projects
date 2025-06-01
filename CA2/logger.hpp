#ifndef LOGGER
#define LOGGER

#include <string>
#include <vector>
using namespace std;
class Logger {
public:
    Logger(std::string program);

    void error(const string& msg);
    void warning(const string& msg);
    void info(const string& msg);
    void perrno();
    void debug(const string& msg);
    void start_message(const vector<string> products);
    void main_msg(const string& msg);
    void update_name(const string name);
    void print_leftovers(const string name,const int count,const int price);
    void print_total_profit(const int msg);
    void info_file(const string &msg);
private:
    std::string program_;
};

#endif
