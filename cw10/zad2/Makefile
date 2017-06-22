CC = gcc
CFLAGS = -W -Wall -std=c99 -pthread -D_GNU_SOURCE

.PHONY: clean client server all

server:
	$(CC) $(CFLAGS) server.c -o server

client:
	$(CC) $(CFLAGS) client.c -o client

all: client server

clean:
	-rm -f client server
