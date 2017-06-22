#ifndef _STACK_H_
#define _STACK_H_

#include <errno.h>
#define MAX_SIZE 20

struct PID_Stack {
  int pids[MAX_SIZE];
  int top;
};

int stack_push (struct PID_Stack *s, int pid);
int stack_pop (struct PID_Stack *s);
int stack_is_empty(struct PID_Stack *s);

#endif
