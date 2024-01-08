kdlib = ../libkdtree.a

CC = gcc
CFLAGS = -pedantic -Wall -g -I..
LDFLAGS = $(kdlib) -lm

.PHONY: all
all: test test2 main

test: test.c $(LDFLAGS)
test2: test2.c $(LDFLAGS)
main: main.c $(LDFLAGS)

.PHONY: clean
clean:
	rm -f test test2
