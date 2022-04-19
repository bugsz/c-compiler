#pragma once

#include <exception>
#include <string>

class parse_error : public std::exception {
public:
    parse_error(const char* msg) : msg(msg) {}
    parse_error(const std::string& msg) : msg(msg) {}
    virtual const char* what() const noexcept { return msg.c_str(); }
private:
    std::string msg;
};