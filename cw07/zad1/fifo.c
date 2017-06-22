#define _GNU_SOURCE

#include "fifo.h"

void write_out_fifo(FIFO_Q* fifo) {
  printf("HEAD: %d\n", fifo->head);
  printf("TAIL: %d\n", fifo->tail);
  printf("CHAIR: %d\n", fifo->chair);
  printf("TAB: ");
  for(int i = 0; i<20; i++)
    printf("%d, ", fifo->tab[i]);
  printf("\n");
}
void init_fifo(FIFO_Q* fifo, int chair_num){
  fifo->max = chair_num;
  fifo->head = -1;
  fifo->tail = 0;
  fifo->chair = 0;
}

int is_empty_fifo(FIFO_Q* fifo){
  if(fifo->head == -1) {
    return 1;
  }
  else {
    return 0;
  }
}

int is_full_fifo(FIFO_Q* fifo){
  if(fifo->head == fifo->tail) {
    return 1;
  }
  else {
    return 0;
  }
}

pid_t pop_fifo(FIFO_Q* fifo){
  if(is_empty_fifo(fifo) == 1) {
    return -1;
  }

  fifo->chair = fifo->tab[fifo->head++];

  if(fifo->head == fifo->max) {
    fifo->head = 0;
  }

  if(fifo->head == fifo->tail) {
    fifo->head = -1;
  }

  return fifo->chair;
}

int push_fifo(FIFO_Q* fifo, pid_t pid){

  if(is_full_fifo(fifo) == 1) {
    return -1;
  }

  if(is_empty_fifo(fifo) == 1){
    fifo->head = fifo->tail;
  }

  fifo->tab[fifo->tail++] = pid;

  if(fifo->tail == fifo->max) {
    fifo->tail = 0;
  }

  return 0;
}
