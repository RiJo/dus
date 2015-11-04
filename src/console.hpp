#ifndef __CONSOLE_HPP_INCLUDED__
#define __CONSOLE_HPP_INCLUDED__

#include <string>
#include <sstream>
#include <stdexcept>

#include <stdio.h>

std::string exec(const std::string &command) {
    FILE* fp = popen(command.c_str(), "r");
    if (fp == nullptr)
        throw std::runtime_error("Failed to open pipe: \"" + command + "\"");

    char buffer[32];
    std::string result = "";
    while (!feof(fp)) {
        if (fgets(buffer, 32, fp) != nullptr)
            result += buffer;
    }
    pclose(fp);
    fp = nullptr;

    return result;
}

class console {
    private:
        void write_char(int x, int y, char c, bool sync) {
            if (x < 0 || x >= cols)
                throw std::out_of_range("X coordinate (" + std::to_string(x) + ") is out of range (0-" + std::to_string(cols) + ").");
            if (y < 0 || y >= rows)
                throw std::out_of_range("Y coordinate (" + std::to_string(y) + ") is out of range (0-" + std::to_string(rows) + ").");

            // TODO: implement
            throw std::runtime_error("write_char() is not yet implemented.");

            //if (sync)
            //    redraw();
        }
    public:
        int cols {0};
        int rows {0};

        console() {
            std::string stty = exec("stty -F /dev/tty size");
            if (stty.length() > 0) {
                std::istringstream st(stty);
                st >> rows;
                st >> cols;
            }
            else {
                rows = 50;
                cols = 100;
            }
        }

        ~console() {
        }

        void write(int x, int y, char c) {
            write_char(x, y, c, true);
        }

        void write(int x, int y, std::string s) {
            for (unsigned int i = 0; i < s.length(); i++)
                write_char(x + i, y, s[i], false);
            //redraw();
        }
};

#endif //__CONSOLE_HPP_INCLUDED__
