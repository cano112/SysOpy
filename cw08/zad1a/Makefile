CC = gcc
CFLAGS = -W -Wall -std=c99 -pthread -D_GNU_SOURCE

.PHONY: clean zad1a all

zad1a:
	$(CC) $(CFLAGS) main.c -o zad1a

file_gen:
	$(CC) $(CFLAGS) file_gen.c -o file-gen

all: zad1a

clean:
	-rm -f zad1a
