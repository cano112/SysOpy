#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "AddressBook.h"


struct _Date
{
  unsigned int day;
  unsigned int month;
  unsigned int year;
};

struct _Person
{
  char *first_name;
  char *last_name;
  Date birth_date;
  char *phone;
  char *email;
  char *address;
};

int compare_people(char *mode, Person *rec1, Person *rec2)
{

  if(strcmp(mode,"phone") == 0)
  {
    return strcmp(rec1->phone,rec2->phone);
  }
  else
  {
    if(strcmp(mode,"birth_date") == 0)
    {
      if(rec1->birth_date.year < rec2->birth_date.year)
        return -1;
      else
      {
        if(rec1->birth_date.year == rec2->birth_date.year)
        {
          if(rec1->birth_date.month < rec2->birth_date.month)
            return -1;
          else
          {
            if(rec1->birth_date.month == rec2->birth_date.month)
            {
              if(rec1->birth_date.day < rec2->birth_date.day)
                return -1;
              else
              {
                if(rec1->birth_date.day == rec2->birth_date.day)
                  return 0;
                else
                  return 1;
              }
            }
            else
              return 1;
          }
        }
        else
          return 1;
      }
    }
    else
    {
      if(strcmp(mode,"email") == 0)
        return strcmp(rec1->email, rec2->email);
      else
      {
        return strcmp(rec1->last_name,rec2->last_name);
      }

    }
  }
}

struct _ListRecord
{
  Person person;
  ListRecord *next;
  ListRecord *previous;
};

struct _ListedAddressBook
{
  ListRecord *head;
  ListRecord *tail;
  unsigned int count;
};

ListedAddressBook *create_listed_address_book()
{
  ListedAddressBook *book = malloc(sizeof(ListedAddressBook));
  book->head = NULL;
  book->tail = NULL;
  book->count = 0;

  return book;
}

void remove_listed_address_book(ListedAddressBook *book)
{
  ListRecord *current = book->head;
  while(current != NULL)
  {
    ListRecord *next = current->next;
    free(current);
    current = next;
  }

  free(book);
}

void add_to_listed_address_book(ListedAddressBook *book, char *first_name,
  char *last_name, int day, int month, int year, char *phone, char *email, char *address)
{
  ListRecord *record = malloc(sizeof(ListRecord));

  record->person.first_name = first_name;
  record->person.last_name = last_name;
  record->person.birth_date.day = day;
  record->person.birth_date.month = month;
  record->person.birth_date.year = year;
  record->person.phone = phone;
  record->person.email = email;
  record->person.address = address;

  record->next = NULL;

  if(book->tail == NULL && book->head == NULL)
  {
    record->previous = NULL;
    book->head = record;
    book->tail = record;
  }
  else
  {
    record->previous = book->tail;
    book->tail->next = record;
    book->tail = record;
  }
  book->count++;
}

void remove_from_listed_address_book(ListedAddressBook *book, ListRecord *record)
{
  if(book->count == 1)
  {
    book->head = NULL;
    book->tail = NULL;
  }
  else
  {
    if(book->tail == record)
    {
      book->tail = record->previous;
      book->tail->next = NULL;
    }
    else
    {
      if(book->head == record)
      {
        book->head = record->next;
        book->head->previous = NULL;
      }
      else
      {
        record->previous->next = record->next;
        record->next->previous = record->previous;
      }
    }
  }

  book->count--;

  free(record);
}

ListRecord *find_record_by_name(ListedAddressBook *book, char *first_name, char *last_name)
{
  ListRecord *current = book->head;
  while(current != NULL && (strcmp(current->person.first_name, first_name) != 0 ||
    strcmp(current->person.last_name, last_name) != 0))
  {
    current = current->next;
  }
  return current;

}

ListedAddressBook *sort_list(ListedAddressBook *book, char *mode)
{
  ListedAddressBook *sorted_book = create_listed_address_book();
  ListRecord *min;
  ListRecord *current;

  while(book->head != NULL)
  {
    min = book->head;
    current = book->head->next;

    while(current != NULL)
    {
      if(compare_people(mode,&(min->person),&(current->person)) > 0)
      {
        min = current;
      }
      current = current->next;
    }
    add_to_listed_address_book(sorted_book, min->person.first_name,
      min->person.last_name, min->person.birth_date.day, min->person.birth_date.month,
      min->person.birth_date.year, min->person.phone, min->person.email,
      min->person.address);

    remove_from_listed_address_book(book, min);
  }

  remove_listed_address_book(book);
  return sorted_book;
}


struct _TreeRecord
{
  TreeRecord *left;
  TreeRecord *right;
  TreeRecord *parent;

  Person person;
};

struct _TreeAddressBook
{
  TreeRecord *head;
  int count;
};

TreeAddressBook *create_tree_address_book()
{
  TreeAddressBook *book = malloc(sizeof(TreeAddressBook));
  book->head = NULL;
  book->count = 0;

  return book;
}

void pre_order_node_delete(TreeRecord *head)
{
  if(head != NULL)
  {
    TreeRecord *tmp_left = head->left;
    TreeRecord *tmp_right = head->right;
    free(head);
    pre_order_node_delete(tmp_left);
    pre_order_node_delete(tmp_right);
  }

}

void remove_tree_address_book(TreeAddressBook *book)
{
  pre_order_node_delete(book->head);
  free(book);
}

void add_to_tree_address_book(TreeAddressBook *book, char *first_name,
  char *last_name, int day, int month, int year, char *phone, char *email, char *address)
{
    TreeRecord *record = malloc(sizeof(TreeRecord));
    record->left = book->head;
    record->right = NULL;
    record->parent = NULL;

    record->person.first_name = first_name;
    record->person.last_name = last_name;
    record->person.birth_date.day = day;
    record->person.birth_date.month = month;
    record->person.birth_date.year = year;
    record->person.phone = phone;
    record->person.email = email;
    record->person.address = address;

    if(book->head != NULL) book->head->parent = record;
    book->head = record;
    book->count++;
}

void remove_from_tree_address_book(TreeAddressBook *book, TreeRecord *record)
{
  if(record != NULL) {
    if(record->left == NULL && record->right == NULL)
    {
        if(record->parent == NULL)
        {
          book->head = NULL;
        }
        else
        {
            if(record->parent->left != NULL && record->parent->left == record)
            {
              record->parent->left = NULL;
            }
            else
            {
              record->parent->right = NULL;
            }
        }
    }
    else
    {

      TreeRecord *tmp_right = record->right;
      record->left->parent = record->parent;
      if(record->parent != NULL) {
        if(record->parent->left != NULL && record->parent->left == record)
        {
          record->parent->left = record->left;
        }
        else
        {
          record->parent->right = record->left;
        }
      }
      else
      {
        book->head = record->left;
      }

      TreeRecord *current = book->head;
      while(current->left != NULL)
      {
        current = current->left;
      }
      current->left = tmp_right;
    }

    free(record);
    book->count--;
  }

}

TreeRecord *search_for_tree_record(TreeRecord *head, char *first_name, char *last_name)
{
  if(head != NULL && (strcmp(head->person.first_name, first_name) != 0 ||
    strcmp(head->person.last_name, last_name) != 0))
  {
    search_for_tree_record(head->left, first_name, last_name);
    search_for_tree_record(head->right, first_name, last_name);
  }
  return head;
}

TreeRecord *find_tree_rec_by_name(TreeAddressBook *book, char *first_name, char *last_name)
{
  return search_for_tree_record(book->head, first_name, last_name);
}

TreeRecord *new_tree_record(char *first_name, char *last_name, int day, int month,
  int year, char *phone, char *email, char *address)
{
    TreeRecord *record = malloc(sizeof(TreeRecord));

    record->left = NULL;
    record->right = NULL;
    record->parent = NULL;

    record->person.first_name = first_name;
    record->person.last_name = last_name;
    record->person.birth_date.day = day;
    record->person.birth_date.month = month;
    record->person.birth_date.year = year;
    record->person.phone = phone;
    record->person.email = email;
    record->person.address = address;

    return record;
}

TreeAddressBook *sort_tree(TreeAddressBook *book, char *mode)
{
    TreeAddressBook *sorted_book = create_tree_address_book();

    while (book->head != NULL)
    {
      TreeRecord *current = sorted_book->head;
      TreeRecord *parent = NULL;
      while(current != NULL) {

        parent = current;
        if(compare_people(mode, &(current->person), &(book->head->person)) > 0)
        {
          current = current->left;
        }
        else
        {
            current = current->right;
        }

      }
      if(parent == NULL)
      {
        sorted_book->head = new_tree_record(book->head->person.first_name,
          book->head->person.last_name, book->head->person.birth_date.day,
          book->head->person.birth_date.month, book->head->person.birth_date.year,
          book->head->person.phone, book->head->person.email,
          book->head->person.address);
      }
      else
      {
        if(compare_people(mode, &(parent->person), &(book->head->person)) > 0)
        {
          parent->left = new_tree_record(book->head->person.first_name,
            book->head->person.last_name, book->head->person.birth_date.day,
            book->head->person.birth_date.month, book->head->person.birth_date.year,
            book->head->person.phone, book->head->person.email,
            book->head->person.address);
          parent->left->parent = parent;
        }
        else {
          parent->right = new_tree_record(book->head->person.first_name,
            book->head->person.last_name, book->head->person.birth_date.day,
            book->head->person.birth_date.month, book->head->person.birth_date.year,
            book->head->person.phone, book->head->person.email,
            book->head->person.address);
          parent->right->parent = parent;

        }
      }
      remove_from_tree_address_book(book, book->head);
    }
    remove_tree_address_book(book);
    return sorted_book;
}
