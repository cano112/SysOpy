CC = gcc
CFLAGS = -W -Wall -std=c99 -D_DEFAULT_SOURCE

.PHONY: clean slave master all

master:
	$(CC) $(CFLAGS) master.c -o master

slave:
		$(CC) $(CFLAGS) slave.c -o slave

all: master slave

clean:
	-rm -f *.o
