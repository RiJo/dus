dus
===

Dump sorted summary list of the current/given directory. Ability to order the result, ascending or descending, based on by name or size.

### Status
Currently in development of first beta version. Current code base works as intended.

## Background
I was missing a command line tool which simply summarized the current directory's size (recursively). This because I'm always running out of disk space when there's no time to spare. I found myself using GNU `du -s` (oh, thereof the name) quite often (no, I don't have any graphical file manager installed); but the result is neither sorted nor quickly interpreted. This application scratches one, out of many, of my personal itches.

## Screenshot
![dus preview](screenshot.png)

## Environment
Everything is written in C++14. There are currently two dependencies which are required to be linked in: libpthread and libcurses.

### Compile
g++ --std=c++14 -pthread -lcurses src/main.cpp

## License
This project is licensed under GPLv3.

