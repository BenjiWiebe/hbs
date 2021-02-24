CFLAGS=-g

all: invent globtest

invent: invent.o
	gcc -g invent.o -o invent -lpcre2-8

invent.o: invent.c

globtest: globtest.o

globtest.o: globtest.c
