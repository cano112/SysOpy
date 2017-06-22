CC = gcc
CFLAGS = -W -Wall -std=c99 -pthread -D_GNU_SOURCE

.PHONY: clean zad2 all

zad2:
	$(CC) $(CFLAGS) main.c -o zad2

file_gen:
	$(CC) $(CFLAGS) file_gen.c -o file-gen

all: zad2

clean:
	-rm -f zad2
