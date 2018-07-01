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
	$(RM) $(PROGRAM) unit-test
.PHONY: clean

install: $(PROGRAM)
	$(INSTALL_BIN) $(PROGRAM) $(DESTDIR)$(BIN_DIR)/$(PROGRAM)
.PHONY: install

uninstall:
	$(RM) $(DESTDIR)$(BIN_DIR)/$(PROGRAM)
.PHONY: uninstall

unit-test: test/test.cpp test/unit.hpp test/test_unit.hpp test/test_thread_pool.hpp $(HEADERS)
	@$(CXX) $(CXXFLAGS) -Itest -Isrc $< -o $@
	@./$@

