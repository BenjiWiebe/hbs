CFLAGS=-g

all: invent

invent: LDLIBS=-lpcre2-8
invent: invent.o json_escape.o

invent.o: invent.c

json_escape.o: json_escape.c

json_escape_test: json_escape.c
	$(CC) -DSTANDALONE_TEST json_escape.c -o json_escape_test

globtest: globtest.o

globtest.o: globtest.c
