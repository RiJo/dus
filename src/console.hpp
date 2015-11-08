#ifndef __CONSOLE_HPP_INCLUDED__
#define __CONSOLE_HPP_INCLUDED__

#include <string>
#include <sstream>
#include <stdexcept>
#include <iostream>

#include <stdio.h>

// Reference: https://en.wikipedia.org/wiki/ANSI_escape_code#Colors
#define ANSI_COLOR_RESET            "\x1b[0;0m"
// Normal
#define ANSI_COLOR_NORMAL_BLACK     "\x1b[0;30m"
#define ANSI_COLOR_NORMAL_RED       "\x1b[0;31m"
#define ANSI_COLOR_NORMAL_GREEN     "\x1b[0;32m"
#define ANSI_COLOR_NORMAL_YELLOW    "\x1b[0;33m"
#define ANSI_COLOR_NORMAL_BLUE      "\x1b[0;34m"
#define ANSI_COLOR_NORMAL_MAGENTA   "\x1b[0;35m"
#define ANSI_COLOR_NORMAL_CYAN      "\x1b[0;36m"
#define ANSI_COLOR_NORMAL_WHITE     "\x1b[0;37m"
// Bright
#define ANSI_COLOR_BRIGHT_BLACK     "\x1b[1;30m"
#define ANSI_COLOR_BRIGHT_RED       "\x1b[1;31m"
#define ANSI_COLOR_BRIGHT_GREEN     "\x1b[1;32m"
#define ANSI_COLOR_BRIGHT_YELLOW    "\x1b[1;33m"
#define ANSI_COLOR_BRIGHT_BLUE      "\x1b[1;34m"
#define ANSI_COLOR_BRIGHT_MAGENTA   "\x1b[1;35m"
#define ANSI_COLOR_BRIGHT_CYAN      "\x1b[1;36m"
#define ANSI_COLOR_BRIGHT_WHITE     "\x1b[1;37m"

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

        std::ostream& black(std::ostream& stream) {
            return stream << ANSI_COLOR_NORMAL_BLACK;
        }

        std::ostream& dark_red(std::ostream& stream) {
            return stream << ANSI_COLOR_NORMAL_RED;
        }

        std::ostream& dark_green(std::ostream& stream) {
            return stream << ANSI_COLOR_NORMAL_GREEN;
        }

        std::ostream& brown(std::ostream& stream) {
            return stream << ANSI_COLOR_NORMAL_YELLOW;
        }

        std::ostream& dark_blue(std::ostream& stream) {
            return stream << ANSI_COLOR_NORMAL_BLUE;
        }

        std::ostream& dark_magenta(std::ostream& stream) {
            return stream << ANSI_COLOR_NORMAL_MAGENTA;
        }

        std::ostream& dark_cyan(std::ostream& stream) {
            return stream << ANSI_COLOR_NORMAL_CYAN;
        }

        std::ostream& gray(std::ostream& stream) {
            return stream << ANSI_COLOR_NORMAL_WHITE;
        }

        std::ostream& dark_gray(std::ostream& stream) {
            return stream << ANSI_COLOR_BRIGHT_BLACK;
        }

        std::ostream& red(std::ostream& stream) {
            return stream << ANSI_COLOR_BRIGHT_RED;
        }

        std::ostream& green(std::ostream& stream) {
            return stream << ANSI_COLOR_BRIGHT_GREEN;
        }

        std::ostream& yellow(std::ostream& stream) {
            return stream << ANSI_COLOR_BRIGHT_YELLOW;
        }

        std::ostream& blue(std::ostream& stream) {
            return stream << ANSI_COLOR_BRIGHT_BLUE;
        }

        std::ostream& magenta(std::ostream& stream) {
            return stream << ANSI_COLOR_BRIGHT_MAGENTA;
        }

        std::ostream& cyan(std::ostream& stream) {
            return stream << ANSI_COLOR_BRIGHT_CYAN;
        }

        std::ostream& white(std::ostream& stream) {
            return stream << ANSI_COLOR_BRIGHT_WHITE;
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
