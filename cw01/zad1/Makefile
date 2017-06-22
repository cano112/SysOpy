CC = gcc

CFLAGS = -W -Wall -std=c99

static:
	$(CC) $(CFLAGS) -c AddressBook.c -o AddressBook.o
	ar cr ../zad2/libAddressBook.a AddressBook.o

shared:
	$(CC) $(CFLAGS) -c -fPIC AddressBook.c -o AdressBook.o
	$(CC) -shared -fPIC -o ../zad2/libAddressBook.so AdressBook.o

.PHONY: clean

clean:
	-rm -f *.o
