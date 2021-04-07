CFLAGS=-g

all: invent vendupd

invent: LDLIBS=-lpcre2-8 -lm
invent: invent.o json_stringify.o

invent.o: invent.c

json_stringify.o: json_stringify.c

clean:
	-rm -f *.o invent

install: invent
	@echo 'Installing to ~/'
	-cp invent ~/
	-cp vermeer_inventory.rb ~/

