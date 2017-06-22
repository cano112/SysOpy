CC = gcc
CFLAGS = -W -Wall -std=c99

.PHONY: clean all zad1 test

test:
	$(CC) $(CFLAGS) test.c -o test
zad1:
	$(CC) $(CFLAGS) main.c -o zad1

all: test zad1

clean:
	-rm -f *.o
