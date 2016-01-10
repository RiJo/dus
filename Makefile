PROGRAM = dus
SOURCES = $(wildcard src/*.cpp)
HEADERS = $(wildcard src/*.hpp)

CXX      ?= g++
CXXFLAGS += -std=c++14 -Wall -Werror -Wextra -Wpedantic -Wshadow
LDLIBS   += -lpthread

INSTALL     = install
INSTALL_BIN = $(INSTALL) -D -m 755

PREFIX  = /usr/local
BIN_DIR = $(PREFIX)/bin

all: $(PROGRAM)
.PHONY: all

$(PROGRAM): $(SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(LDLIBS) $< -o $@

debug: CXXFLAGS += -g -D=DEBUG
debug: all

clean:
	$(RM) $(PROGRAM)
.PHONY: clean

install: $(PROGRAM)
	$(INSTALL_BIN) $(PROGRAM) $(DESTDIR)$(BIN_DIR)/$(PROGRAM)
.PHONY: install

uninstall:
	$(RM) $(DESTDIR)$(BIN_DIR)/$(PROGRAM)
.PHONY: uninstall
