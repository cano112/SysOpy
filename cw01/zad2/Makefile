CC = gcc
CFLAGS = -W -Wall -O$(opt)

list_static:
	$(CC) $(CFLAGS) list_test_1.c -o list_test_static -L. libAddressBook.a
	./list_test_static

list_shared:
		$(CC) $(CFLAGS) list_test_1.c -o list_test_shared -L. -Wl,-rpath=. libAddressBook.so
		./list_test_shared

list_dynamic:
	$(CC) $(CFLAGS) -rdynamic list_test_2.c -o list_test_dynamic -ldl
	./list_test_dynamic

tree_static:
	$(CC) $(CFLAGS) tree_test_1.c -o tree_test_static -L. libAddressBook.a
	./tree_test_static

tree_shared:
	$(CC) $(CFLAGS) tree_test_1.c -o tree_test_shared -L. -Wl,-rpath=. libAddressBook.so
	./tree_test_shared

tree_dynamic:
	$(CC) $(CFLAGS) -rdynamic tree_test_2.c -o tree_test_dynamic -ldl
	./tree_test_dynamic

.PHONY: clean

clean:
	-rm -f *.o
