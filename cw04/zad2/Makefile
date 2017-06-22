CC = gcc
CFLAGS = -W -Wall -std=c99 -D_POSIX_C_SOURCE=199309L

.PHONY: clean zad2 all

zad2:
	$(CC) $(CFLAGS) main.c stack.c -o zad2

all: zad2

clean:
	-rm -f *.o
