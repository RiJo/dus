#ifndef __CONSOLE_HPP_INCLUDED__
#define __CONSOLE_HPP_INCLUDED__

#include <string>
#include <sstream>
#include <stdexcept>
#include <iostream>

#include <stdio.h>

#define ANSI_COLOR_BLACK    "\x1b[30m"
#define ANSI_COLOR_RED      "\x1b[31m"
#define ANSI_COLOR_GREEN    "\x1b[32m"
#define ANSI_COLOR_YELLOW   "\x1b[33m"
//#define ANSI_COLOR_BROWN   "\x1b[33m"
#define ANSI_COLOR_BLUE     "\x1b[34m"
#define ANSI_COLOR_MAGENTA  "\x1b[35m"
//#define ANSI_COLOR_PURPLE  "\x1b[35m"
#define ANSI_COLOR_CYAN     "\x1b[36m"
#define ANSI_COLOR_RESET    "\x1b[0m"

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

namespace console {
    namespace color {
        std::ostream& reset(std::ostream& stream) {
            return stream << ANSI_COLOR_RESET;
        }

        std::ostream& red(std::ostream& stream) {
            return stream << ANSI_COLOR_RED;
        }

        std::ostream& green(std::ostream& stream) {
            return stream << ANSI_COLOR_GREEN;
        }

        std::ostream& yellow(std::ostream& stream) {
            return stream << ANSI_COLOR_YELLOW;
        }

        std::ostream& blue(std::ostream& stream) {
            return stream << ANSI_COLOR_BLUE;
        }

        std::ostream& magenta(std::ostream& stream) {
            return stream << ANSI_COLOR_MAGENTA;
        }

        std::ostream& cyan(std::ostream& stream) {
            return stream << ANSI_COLOR_CYAN;
        }
    }

    class tty {
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

            tty() {
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

            ~tty() {
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
}

#endif //__CONSOLE_HPP_INCLUDED__
