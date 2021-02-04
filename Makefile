CFLAGS=-g

all: invent

invent: invent.o
	gcc -g invent.o -o invent -lpcre2-8

invent.o: invent.c
