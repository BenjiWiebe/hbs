CFLAGS=-g

all: invent pcre

invent: invent.o
	gcc -g invent.o -o invent -lpcre2-8

invent.o: invent.c
