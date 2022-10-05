CC = gcc
# Release flags
RELFLAGS = -Wall -I./include/
# Debug flags
DBGFLAGS = -Wall -I./include/ -O0 -g
EXEC = epsilon

SRCMODULES = core/errors.c core/input.c core/memory.c \
			 core/ds/list.c core/ds/dict.c core/object.c\
			 lexer/lexer.c lexer/token.c \
			 parser/parser.c \
			 interpreter/interpret.c interpreter/enviroment.c \
			 interpreter/statements.c interpreter/expressions.c \
			 interpreter/runtime_errors.c

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
