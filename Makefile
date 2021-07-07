CFLAGS=-g

all: invent vendupd

invent: LDLIBS=-lpcre2-8 -lm -lsqlite3
invent: invent.o json_stringify.o update_db.o

invent.o: invent.c

update_db.o: update_db.c

json_stringify.o: json_stringify.c

clean:
	-rm -f *.o invent

install: invent
	@echo 'Installing to ~/'
	-cp invent ~/
	-cp vermeer_inventory.rb ~/

