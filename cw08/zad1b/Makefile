CC = gcc
CFLAGS = -W -Wall -std=c99 -pthread -D_GNU_SOURCE

.PHONY: clean zad1b all

zad1b:
	$(CC) $(CFLAGS) main.c -o zad1b

file_gen:
	$(CC) $(CFLAGS) file_gen.c -o file-gen

all: zad1b

clean:
	-rm -f zad1b
