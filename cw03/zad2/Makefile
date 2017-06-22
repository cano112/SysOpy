CC = gcc
CFLAGS = -W -Wall -std=c99

.PHONY: clean mem_test cpu_test zad2 all

mem_test:
	$(CC) $(CFLAGS) mem_test.c -o mem_test

cpu_test:
	$(CC) $(CFLAGS) cpu_test.c -o cpu_test

zad2:
	$(CC) $(CFLAGS) main.c -o zad2

all: zad2 cpu_test mem_test

clean:
	-rm -f *.o
