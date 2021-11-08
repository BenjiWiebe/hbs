CFLAGS=-g
CC=gcc -std=c99

all: invent

invent: LDLIBS=-lpcre2-8 -lm -lsqlite3
invent: invent.o json_stringify.o update_db.o print_entry_json.o

invent.o: invent.c

update_db.o: update_db.c

json_stringify.o: json_stringify.c

print_entry_json.o: print_entry_json.c

clean:
	-rm -f *.o invent

install: invent
	@echo 'Installing to ~/'
	-cp invent ~/
	-cp vermeer_inventory.rb ~/

