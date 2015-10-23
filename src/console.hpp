#ifndef __CONSOLE_HPP_INCLUDED__
#define __CONSOLE_HPP_INCLUDED__

#include <stdexcept>
#include <curses.h>

class console {
    private:
        WINDOW *wnd {nullptr};

        void write_char(int x, int y, char c, bool sync) {
            if (x < 0 || x >= cols)
                throw std::out_of_range("X coordinate (" + std::to_string(x) + ") is out of range (0-" + std::to_string(rows) + ").");
            if (y < 0 || y >= rows)
                throw std::out_of_range("Y coordinate (" + std::to_string(y) + ") is out of range (0-" + std::to_string(rows) + ").");

            mvwaddch(wnd, y, x, c);

            if (sync)
                refresh();
        }
    public:
        int cols {0};
        int rows {0};

        console() {
            wnd = initscr();
            //wnd = stdscr;
            cbreak();
            noecho();
            getmaxyx(stdscr, rows, cols);
            //keypad(wnd, TRUE);
            //nodelay(wnd, TRUE); // getch() is not blocking
        }

        ~console() {
            endwin();
            wnd = nullptr;
        }

        void write(int x, int y, char c) {
            write_char(x, y, c, true);
        }

        void write(int x, int y, std::string s) {
            for (int i = 0; i < s.length(); i++)
                write_char(x + i, y, s[i], false);
            refresh();
        }
};

#endif //__CONSOLE_HPP_INCLUDED__
