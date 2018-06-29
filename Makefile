PROGRAM ?= dus
SOURCES = $(wildcard src/*.cpp)
HEADERS = $(wildcard src/*.hpp)

CXX      ?= g++
CXXFLAGS += -std=c++17 -Wall -Werror -Wextra -Wpedantic -Wshadow -pthread
LDLIBS   += 

INSTALL     ?= install
INSTALL_BIN = $(INSTALL) -D -m 755

PREFIX  = /usr/local
BIN_DIR = $(PREFIX)/bin

all: Makefile $(PROGRAM)
.PHONY: all

$(PROGRAM): $(SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(LDLIBS) $< -o $@

debug: CXXFLAGS += -g -D=DEBUG
debug: all
.PHONY: debug

clean:
	$(RM) $(PROGRAM)
.PHONY: clean

install: $(PROGRAM)
	$(INSTALL_BIN) $(PROGRAM) $(DESTDIR)$(BIN_DIR)/$(PROGRAM)
.PHONY: install

uninstall:
	$(RM) $(DESTDIR)$(BIN_DIR)/$(PROGRAM)
.PHONY: uninstall

unit-test: test/thread_pool
	@./test/thread_pool

test/thread_pool: test/thread_pool.cpp test/unit.hpp $(HEADERS)
	@$(CXX) $(CXXFLAGS) -Itest -Isrc $< -o $@

