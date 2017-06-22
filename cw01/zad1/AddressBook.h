#ifndef _ADDRESSBOOK_H_
#define _ADDRESSBOOK_H_

/*
GENERAL
*/
typedef struct _Date Date;

typedef struct _Person Person;

int compare_people(char*, Person*, Person*);


/*
LISTED ADDRESS BOOK
*/
typedef struct _ListedAddressBook ListedAddressBook;

typedef struct _ListRecord ListRecord;

ListedAddressBook *create_listed_address_book();

void remove_listed_address_book(ListedAddressBook*);

void add_to_listed_address_book(ListedAddressBook*, char*, char*, int, int, int, char*, char*, char*);

void remove_from_listed_address_book(ListedAddressBook*, ListRecord*);

ListRecord *find_record_by_name(ListedAddressBook*, char*, char*);

ListedAddressBook *sort_list(ListedAddressBook*, char*);


/*
TREE ADDRESS BOOK
*/
typedef struct _TreeAddressBook TreeAddressBook;

typedef struct _TreeRecord TreeRecord;

TreeAddressBook *create_tree_address_book();

void remove_tree_address_book(TreeAddressBook*);

void add_to_tree_address_book(TreeAddressBook*, char*, char*, int, int, int, char*, char*, char*);

void remove_from_tree_address_book(TreeAddressBook*, TreeRecord*);

TreeRecord *find_tree_rec_by_name(TreeAddressBook*, char*, char*);

TreeAddressBook *sort_tree(TreeAddressBook*, char*);

#endif
