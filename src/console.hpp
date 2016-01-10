#ifndef __CONSOLE_HPP_INCLUDED__
#define __CONSOLE_HPP_INCLUDED__

#include <string>
#include <sstream>
#include <stdexcept>
#include <iostream>
#include <vector>

#include <stdio.h>

// Reference: https://en.wikipedia.org/wiki/ANSI_escape_code#Colors
#define ANSI_COLOR_FOREGROUND_RESET            "\x1b[0;0m"
// Foreground - Normal
#define ANSI_COLOR_FOREGROUND_NORMAL_BLACK     "\x1b[0;30m"
#define ANSI_COLOR_FOREGROUND_NORMAL_RED       "\x1b[0;31m"
#define ANSI_COLOR_FOREGROUND_NORMAL_GREEN     "\x1b[0;32m"
#define ANSI_COLOR_FOREGROUND_NORMAL_YELLOW    "\x1b[0;33m"
#define ANSI_COLOR_FOREGROUND_NORMAL_BLUE      "\x1b[0;34m"
#define ANSI_COLOR_FOREGROUND_NORMAL_MAGENTA   "\x1b[0;35m"
#define ANSI_COLOR_FOREGROUND_NORMAL_CYAN      "\x1b[0;36m"
#define ANSI_COLOR_FOREGROUND_NORMAL_WHITE     "\x1b[0;37m"
// Foreground - Bright
#define ANSI_COLOR_FOREGROUND_BRIGHT_BLACK     "\x1b[1;30m"
#define ANSI_COLOR_FOREGROUND_BRIGHT_RED       "\x1b[1;31m"
#define ANSI_COLOR_FOREGROUND_BRIGHT_GREEN     "\x1b[1;32m"
#define ANSI_COLOR_FOREGROUND_BRIGHT_YELLOW    "\x1b[1;33m"
#define ANSI_COLOR_FOREGROUND_BRIGHT_BLUE      "\x1b[1;34m"
#define ANSI_COLOR_FOREGROUND_BRIGHT_MAGENTA   "\x1b[1;35m"
#define ANSI_COLOR_FOREGROUND_BRIGHT_CYAN      "\x1b[1;36m"
#define ANSI_COLOR_FOREGROUND_BRIGHT_WHITE     "\x1b[1;37m"
// Background
#define ANSI_COLOR_BACKGROUND_BLACK            "\x1b[1;40m"
#define ANSI_COLOR_BACKGROUND_RED              "\x1b[1;41m"
#define ANSI_COLOR_BACKGROUND_GREEN            "\x1b[1;42m"
#define ANSI_COLOR_BACKGROUND_YELLOW           "\x1b[1;43m"
#define ANSI_COLOR_BACKGROUND_BLUE             "\x1b[1;44m"
#define ANSI_COLOR_BACKGROUND_MAGENTA          "\x1b[1;45m"
#define ANSI_COLOR_BACKGROUND_CYAN             "\x1b[1;46m"
#define ANSI_COLOR_BACKGROUND_WHITE            "\x1b[1;47m"

// TODO: move into console namespace
struct exec_result_t {
    int exit_code;
    std::string stdout;
};

// TODO: move into console namespace
exec_result_t exec(const std::string &command) {
    FILE* fp = popen(command.c_str(), "r");
    if (fp == nullptr)
        throw std::runtime_error("Failed to open pipe: \"" + command + "\"");

    exec_result_t result { -1, "" };

    char buffer[32];
    while (!feof(fp)) {
        if (fgets(buffer, 32, fp) != nullptr)
            result.stdout += buffer;
    }

    int temp = pclose(fp);
    fp = nullptr;

    result.exit_code = WEXITSTATUS(temp);
    return result;
}

namespace console {
    namespace color {
        static bool enable {true};

        inline const std::string get_color(const char *ansi_color) {
            if (!enable)
                return "";
            return std::string(ansi_color);
        }

        inline const std::string reset() {
            return get_color(ANSI_COLOR_FOREGROUND_RESET);
        }

        inline const std::string black() {
            return get_color(ANSI_COLOR_FOREGROUND_NORMAL_BLACK);
        }

        inline const std::string dark_red() {
            return get_color(ANSI_COLOR_FOREGROUND_NORMAL_RED);
        }

        inline const std::string dark_green() {
            return get_color(ANSI_COLOR_FOREGROUND_NORMAL_GREEN);
        }

        inline const std::string brown() {
            return get_color(ANSI_COLOR_FOREGROUND_NORMAL_YELLOW);
        }

        inline const std::string dark_blue() {
            return get_color(ANSI_COLOR_FOREGROUND_NORMAL_BLUE);
        }

        inline const std::string dark_magenta() {
            return get_color(ANSI_COLOR_FOREGROUND_NORMAL_MAGENTA);
        }

        inline const std::string dark_cyan() {
            return get_color(ANSI_COLOR_FOREGROUND_NORMAL_CYAN);
        }

        inline const std::string gray() {
            return get_color(ANSI_COLOR_FOREGROUND_NORMAL_WHITE);
        }

        inline const std::string dark_gray() {
            return get_color(ANSI_COLOR_FOREGROUND_BRIGHT_BLACK);
        }

        inline const std::string red() {
            return get_color(ANSI_COLOR_FOREGROUND_BRIGHT_RED);
        }

        inline const std::string green() {
            return get_color(ANSI_COLOR_FOREGROUND_BRIGHT_GREEN);
        }

        inline const std::string yellow() {
            return get_color(ANSI_COLOR_FOREGROUND_BRIGHT_YELLOW);
        }

        inline const std::string blue() {
            return get_color(ANSI_COLOR_FOREGROUND_BRIGHT_BLUE);
        }

        inline const std::string magenta() {
            return get_color(ANSI_COLOR_FOREGROUND_BRIGHT_MAGENTA);
        }

        inline const std::string cyan() {
            return get_color(ANSI_COLOR_FOREGROUND_BRIGHT_CYAN);
        }

        inline const std::string white() {
            return get_color(ANSI_COLOR_FOREGROUND_BRIGHT_WHITE);
        }


        inline std::ostream& colorize(std::ostream& stream, const char *ansi_color) {
            if (!enable)
                return stream;
            return stream << ansi_color;
        }

        inline std::ostream& reset(std::ostream& stream) {
            return colorize(stream, ANSI_COLOR_FOREGROUND_RESET);
        }

        inline std::ostream& black(std::ostream& stream) {
            return colorize(stream, ANSI_COLOR_FOREGROUND_NORMAL_BLACK);
        }

        inline std::ostream& dark_red(std::ostream& stream) {
            return colorize(stream, ANSI_COLOR_FOREGROUND_NORMAL_RED);
        }

        inline std::ostream& dark_green(std::ostream& stream) {
            return colorize(stream, ANSI_COLOR_FOREGROUND_NORMAL_GREEN);
        }

        inline std::ostream& brown(std::ostream& stream) {
            return colorize(stream, ANSI_COLOR_FOREGROUND_NORMAL_YELLOW);
        }

        inline std::ostream& dark_blue(std::ostream& stream) {
            return colorize(stream, ANSI_COLOR_FOREGROUND_NORMAL_BLUE);
        }

        inline std::ostream& dark_magenta(std::ostream& stream) {
            return colorize(stream, ANSI_COLOR_FOREGROUND_NORMAL_MAGENTA);
        }

        inline std::ostream& dark_cyan(std::ostream& stream) {
            return colorize(stream, ANSI_COLOR_FOREGROUND_NORMAL_CYAN);
        }

        inline std::ostream& gray(std::ostream& stream) {
            return colorize(stream, ANSI_COLOR_FOREGROUND_NORMAL_WHITE);
        }

        inline std::ostream& dark_gray(std::ostream& stream) {
            return colorize(stream, ANSI_COLOR_FOREGROUND_BRIGHT_BLACK);
        }

        inline std::ostream& red(std::ostream& stream) {
            return colorize(stream, ANSI_COLOR_FOREGROUND_BRIGHT_RED);
        }

        inline std::ostream& green(std::ostream& stream) {
            return colorize(stream, ANSI_COLOR_FOREGROUND_BRIGHT_GREEN);
        }

        inline std::ostream& yellow(std::ostream& stream) {
            return colorize(stream, ANSI_COLOR_FOREGROUND_BRIGHT_YELLOW);
        }

        inline std::ostream& blue(std::ostream& stream) {
            return colorize(stream, ANSI_COLOR_FOREGROUND_BRIGHT_BLUE);
        }

        inline std::ostream& magenta(std::ostream& stream) {
            return colorize(stream, ANSI_COLOR_FOREGROUND_BRIGHT_MAGENTA);
        }

        inline std::ostream& cyan(std::ostream& stream) {
            return colorize(stream, ANSI_COLOR_FOREGROUND_BRIGHT_CYAN);
        }

        inline std::ostream& white(std::ostream& stream) {
            return colorize(stream, ANSI_COLOR_FOREGROUND_BRIGHT_WHITE);
        }
    }

    struct arg_t {
        std::string key;
        std::string value;
        arg_t *next;
    };

    unsigned int text_width(const std::string &str) {
        unsigned int length = 0;
        for (unsigned int i = 0; i < str.length(); i++) {
            unsigned char c = str[i];

            // UTF-8 multibyte
            if (c > 127) {
                if ((c & 0xE0) == 0xC0)
                    i += 1; // 2 byte sequence
                else if ((c & 0xF0) == 0xE0)
                    i += 2; // 3 byte sequence
                else if ((c & 0xF8) == 0xF0)
                    i += 3; // 4 byte sequence
                else if ((c & 0xFC) == 0xF8)
                    i += 4; // 5 byte sequence
                else if ((c & 0xFE) == 0xFC)
                    i += 5; // 6 byte sequence
                else
                    throw std::runtime_error("Not valid UTF-8 (offset " + std::to_string(i) + "), probably ISO-8859-1: \"" + str + "\"");
            }

            // ANSI escape: \x1b[...m
            if (c == 27) {
                if (i < str.length() - 1 && str[i + 1] == 91) {
                    std::string::size_type offset_m = str.find("m", i + 2);
                    if(offset_m == std::string::npos)
                        throw std::runtime_error("Could not find ANSI escape sequence (offset " + std::to_string(i) + ") end symbol: \"" + str + "\"");
                    i = offset_m + 1;
                }
            }

            length++;
        }
        return length;
    }

    const std::vector<arg_t> parse_args(const int argc, const char *argv[]) {
        std::vector<arg_t> args;

        for (int i = 1; i < argc; i++) {
            std::string arg = std::string(argv[i]);
            switch (arg.length()) {
                case 1:
                    args.emplace_back(arg_t{arg, "", nullptr});
                case 0:
                    continue;
                default:
                    break;
            }

            if (arg[0] != '-') {
                // No option
                args.emplace_back(arg_t{arg, "", nullptr});
                continue;
            }

            if (arg[1] != '-') {
                // Flag(s) option
                for (unsigned int f = 1; f < arg.length(); f++)
                    args.emplace_back(arg_t{"-" + arg.substr(f, 1), "", nullptr});
                continue;
            }

            std::string::size_type offset_equal_sign = arg.find("=", 2);
            if(offset_equal_sign != std::string::npos) {
                // Key-value option
                args.emplace_back(arg_t{arg.substr(0, offset_equal_sign), arg.substr(offset_equal_sign + 1), nullptr});
            } else {
                // Flag option
                args.emplace_back(arg_t{arg, "", nullptr});
            }
        }

        // Link list
        arg_t *prev {nullptr};
        for (auto &arg: args) {
            if (prev)
                prev->next = &arg;
            prev = &arg;
        }

        return args;
    }

    class tty {
        private:
            void write_char(int x, int y, char c, bool sync) {
                if (x < 0 || x >= cols)
                    throw std::out_of_range("X coordinate (" + std::to_string(x) + ") is out of range (0-" + std::to_string(cols) + ").");
                if (y < 0 || y >= rows)
                    throw std::out_of_range("Y coordinate (" + std::to_string(y) + ") is out of range (0-" + std::to_string(rows) + ").");

                // TODO: implement
                (void)c;
                (void)sync;
                throw std::runtime_error("write_char() is not yet implemented.");

                //if (sync)
                //    redraw();
            }
        public:
            int cols {0};
            int rows {0};

            tty() {
                std::string stty = exec("stty -F /dev/tty size").stdout;
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
