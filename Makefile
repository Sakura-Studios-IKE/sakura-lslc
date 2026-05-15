# lslc - portable Makefile (Linux, macOS, *BSD, MinGW)
#
# Usage:
#   make            - build ./lslc
#   make CC=clang   - choose compiler
#   make debug      - build with -g -O0
#   make install    - install to PREFIX (default /usr/local)
#   make test       - run the in-tree test scripts
#
PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin

CC     ?= cc
CFLAGS ?= -O2 -std=c99 -Wall -Wextra -Wpedantic -Wno-unused-parameter \
          -Wno-unused-but-set-variable -Wno-unused-variable -Wno-unused-function
LDFLAGS ?=

SRC := $(wildcard src/*.c)
OBJ := $(SRC:.c=.o)
EXE := lslc

ifeq ($(OS),Windows_NT)
  EXE := lslc.exe
endif

.PHONY: all clean debug install uninstall test help

all: $(EXE)

$(EXE): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

src/%.o: src/%.c src/lsl.h
	$(CC) $(CFLAGS) -c $< -o $@

debug:
	$(MAKE) CFLAGS="-O0 -g -std=c99 -Wall -Wextra"

clean:
	rm -f $(OBJ) $(EXE)

install: $(EXE)
	install -d $(DESTDIR)$(BINDIR)
	install -m 0755 $(EXE) $(DESTDIR)$(BINDIR)/$(EXE)
	install -d $(DESTDIR)$(PREFIX)/share/man/man1
	install -m 0644 man/$(EXE).1 $(DESTDIR)$(PREFIX)/share/man/man1/$(EXE).1

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/$(EXE)
	rm -f $(DESTDIR)$(PREFIX)/share/man/man1/$(EXE).1

test: $(EXE)
	@sh tests/run_tests.sh ./$(EXE)

help:
	@echo "make           build $(EXE)"
	@echo "make debug     build with -g -O0"
	@echo "make test      run regression tests"
	@echo "make install   install to PREFIX=$(PREFIX)"
	@echo "make clean     remove build artifacts"
