CC = gcc
CFLAGS = -W -Wall -std=c99

.PHONY: clean barber customers all

barber:
	$(CC) $(CFLAGS) barber.c fifo.c -o barber

customers:
	$(CC) $(CFLAGS) customers.c fifo.c -o customers

all: barber customers

clean:
	-rm -f *.o
