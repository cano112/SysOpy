#ifndef _FIFO_H_
#define _FIFO_H_

#include <stdlib.h>
#include <sys/types.h>
#include <stdio.h>

typedef struct FIFO_Q{
  int max;
  int head;
  int tail;
  pid_t chair;
  pid_t tab[1000];
} FIFO_Q;

void write_out_fifo(FIFO_Q*);
void init_fifo(FIFO_Q*, int);
pid_t pop_fifo(FIFO_Q*);
int push_fifo(FIFO_Q*, pid_t);
int is_full_fifo(FIFO_Q*);
int is_empty_fifo(FIFO_Q*);

#endif
