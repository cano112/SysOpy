CC = gcc
CFLAGS = -W -Wall -std=c99 -D_GNU_SOURCE

.PHONY: clean server client all

server:
	$(CC) $(CFLAGS) server.c -o server

client:
	$(CC) $(CFLAGS) client.c -o client

all: server client

clean:
	-rm -f *.o
