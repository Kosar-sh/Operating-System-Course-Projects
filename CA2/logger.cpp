#include "logger.hpp"

#include <cerrno>
#include <cstring>
#include <iostream>

#include "colorprint.hpp"

Logger::Logger(std::string program) : program_(std::move(program)) {}

void Logger::error(const std::string& msg) {
    std::cerr << Color::RED << "[ERR:" << program_ << "] "
              << Color::RST << msg << '\n';
}

void Logger::warning(const std::string& msg) {
    std::cout << Color::YEL << "[WRN:" << program_ << "] "
              << Color::RST << msg << '\n';
}

void Logger::info(const std::string& msg) {
    std::cout << Color::BLU << "[" << program_ << "] "
              << Color::RST << msg << '\n';
}

void Logger::perrno() {
    error(strerror(errno));
}

void Logger::debug(const string &msg)
{
    cout<< Color::BLU << "[INF:" << program_ << "] "
              << Color::RST << msg<<endl;
}

void Logger::start_message(const vector<string> products)
{
    cout << Color::CYN  <<"[INF:" << program_ << "] "<< Color::RST
    << "The available products:"<<endl;
    for (int i = 0; i < products.size(); i++)
    {
        cout<<(i+1)<<"-"<<products[i]<<endl;
    }
    cout << "Choose the products you want and for more than 1 input split them with space: ";
}

void Logger::main_msg(const string &msg)
{
        cout << Color::CYN  <<"[" << program_ << "] "<< Color::RST<<msg<<endl;

}

void Logger::update_name(const string name)
{
    this->program_=this->program_+" "+name;
}

void Logger::print_leftovers(const string name, const int count, const int price)
{
        cout<<Color::MAG<<name<<endl;
        cout<< Color::GRN <<"\tTotal Leftover quantity: "<<Color::RST<<count<<endl;
        cout<< Color::GRN <<"\tTotal Leftover price: "<<Color::RST<<price<<endl;

}

void Logger::print_total_profit(const int msg)
{
    cout << Color::CYN  <<"[" << program_ << "] "<< 
    Color::YEL<<"The Total Profit For Products is "<<
    Color::RST<<msg<<endl;
}

void Logger::info_file(const string &msg)
{
    cout<<Color::CYN  <<"[" << program_ << "] "<< Color::RST
    <<"The Pipe Was Built For product "<<msg<<endl;
}
