#ifndef __CONSOLE_HPP_INCLUDED__
#define __CONSOLE_HPP_INCLUDED__

#include <string>
#include <sstream>
#include <stdexcept>

#include <stdio.h>
//#include <curses.h>

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
        //WINDOW *wnd {nullptr};

        void write_char(int x, int y, char c, bool sync) {
            if (x < 0 || x >= cols)
                throw std::out_of_range("X coordinate (" + std::to_string(x) + ") is out of range (0-" + std::to_string(cols) + ").");
            if (y < 0 || y >= rows)
                throw std::out_of_range("Y coordinate (" + std::to_string(y) + ") is out of range (0-" + std::to_string(rows) + ").");

            // TODO: implement
            throw std::runtime_error("write_char() is not yet implemented.");
            //mvwaddch(wnd, y, x, c);

            //if (sync)
            //    refresh();
        }
    public:
        int cols {0};
        int rows {0};

        console() {
            //wnd = initscr();
            //wnd = stdscr;
            //cbreak();
            //noecho();
            //getmaxyx(stdscr, rows, cols);
            //keypad(wnd, TRUE);
            //nodelay(wnd, TRUE); // getch() is not blocking

            std::istringstream st(exec("stty size"));
            st >> rows;
            st >> cols;
        }

        ~console() {
            //endwin();
            //wnd = nullptr;
        }

        void write(int x, int y, char c) {
            write_char(x, y, c, true);
        }

        void write(int x, int y, std::string s) {
            for (int i = 0; i < s.length(); i++)
                write_char(x + i, y, s[i], false);
            //refresh();
        }
};

#endif //__CONSOLE_HPP_INCLUDED__
