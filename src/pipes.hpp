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
