#ifndef __PIPES_HPP_INCLUDED__
#define __PIPES_HPP_INCLUDED__

#include <iostream>

#include <sys/poll.h>

namespace pipes {
    bool stdin_has_data(int timeout = 0) {
        struct pollfd poller;
        poller.fd = STDIN_FILENO;
        poller.events = POLLIN;
        poller.revents = 0;
        return poll(&poller, 1, timeout) > 0;
    }

    std::vector<std::string> read_stdin(char delimiter) {
        std::vector<std::string> result;
        if (!stdin_has_data())
            return result;

        const int buffer_size = 256;
        char buffer[buffer_size];
        while(!std::cin.eof()){
            std::cin.getline(buffer, buffer_size, delimiter);
            result.push_back(std::move(std::string(buffer)));
        }
        return result;
    }

    std::string read_stdin() {
        if (!stdin_has_data())
            return "";

        std::string result {""};
        char c;
        while(!(std::cin.get(c)).eof())
            result += c;
        return result;
    }
}

#endif //__PIPES_HPP_INCLUDED__
