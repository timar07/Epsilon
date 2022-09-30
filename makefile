CC = gcc
# Release flags
RELFLAGS = -Wall -I./include/ -O3
# Debug flags
DBGFLAGS = -Wall -I./include/ -O0 -g -D EPS_DBG
EXEC = epsilon

SRCMODULES = core/errors.c core/input.c core/memory.c \
			 core/ds/list.c core/ds/dict.c core/object.c\
			 lexer/lexer.c \
			 parser/parser.c \
			 interpreter/interpret.c interpreter/enviroment.c

OBJMODULES = $(SRCMODULES:.c=.o)

.DEFAULT_GOAL := all
.PHONY: all clean build install debug

DEBUG ?= 0
ifeq ($(DEBUG), 1)
    CFLAGS := $(DBGFLAGS)
else
    CFLAGS := $(RELFLAGS)
endif

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $< -o ./bin/$@

build: epsilon.c $(OBJMODULES)
	$(CC) $(CFLAGS) $^ -o ./bin/$(EXEC)

clean:
	rm -f ./$(OBJMODULES)

install:
	install ./bin/$(EXEC) /usr/local/bin

all:
	mkdir -p bin
	make build
	make install
	make clean
