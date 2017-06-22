#include "stack.h"

int stack_push (struct PID_Stack *s, int pid) {
    if (s->top == (MAX_SIZE - 1)) {errno = ENOBUFS; return -1; }
    s->pids[++(s->top)] = pid;
    return 0;
}

int stack_pop (struct PID_Stack *s) {
    if (s->top == - 1) {errno = ENODATA; return -1; }
    return s->pids[(s->top)--];
}

int stack_is_empty(struct PID_Stack *s) {
  if(s->top == -1) return 1;
  return 0;
}
