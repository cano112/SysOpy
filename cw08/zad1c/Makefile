CC = gcc
CFLAGS = -W -Wall -std=c99 -pthread -D_GNU_SOURCE

.PHONY: clean zad1c all

zad1c:
	$(CC) $(CFLAGS) main.c -o zad1c

file_gen:
	$(CC) $(CFLAGS) file_gen.c -o file-gen

all: zad1a

clean:
	-rm -f zad1c
