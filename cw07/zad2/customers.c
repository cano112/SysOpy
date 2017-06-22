#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/msg.h>
#include <ctype.h>
#include <time.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <sys/mman.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "fifo.h"
#include "commons.h"

FIFO_Q* fifo = NULL;

sem_t* barber_sem;
sem_t* fifo_sem;
sem_t* barbering_sem;
sem_t* free_sem;

volatile int cuts_count = 0;
sigset_t full_mask;

void int_handler(int signo){
  exit(signo);
}

void rtmin_handler(int signo){
  cuts_count++;
}

void exit_handler(void) {
  if(munmap(fifo, sizeof(fifo)) == -1) printf("Error detaching shared memory\n");
  else printf("Shared memory detached\n");

  if(sem_close(barber_sem) == -1) printf("Error closing semaphore");
  else printf("Semaphore closed\n");
  if(sem_close(fifo_sem) == -1) printf("Error closing semaphore");
  else printf("Semaphore closed\n");
  if(sem_close(barbering_sem) == -1) printf("Error closing semaphore");
  else printf("Semaphore closed\n");
  if(sem_close(free_sem) == -1) printf("Error closing semaphore");
  else printf("Semaphore closed\n");
}

long get_time(){
  struct timespec checkpoint;
  if(clock_gettime(CLOCK_MONOTONIC, &checkpoint) == -1) {
    printf("Failed to get time");
    exit(1);
  }
  return checkpoint.tv_nsec / 1000;
}

void prepare_fifo(){
  int shm_id = shm_open(shm_path, O_RDWR, 0666);
  if(shm_id == -1) {
    printf("Failed to get shared memory id\n");
    exit(1);
  }

  void* shm_ptr = mmap(NULL, sizeof(FIFO_Q), PROT_READ | PROT_WRITE, MAP_SHARED, shm_id, 0);

  if(shm_ptr == (void*)(-1)) {
    printf("Failed to get shared memory pointer\n");
    exit(1);
  }

  fifo = (FIFO_Q*)shm_ptr;
}

void prepare_semaphores() {
  barber_sem = sem_open(barber_path, O_RDWR);
  if(barber_sem == SEM_FAILED) {
    printf("Failed to open semaphore\n");
    exit(1);
  }

  barbering_sem = sem_open(barbering_path, O_RDWR);
  if(barbering_sem == SEM_FAILED) {
    printf("Failed to open semaphore\n");
    exit(1);
  }

  fifo_sem = sem_open(fifo_path, O_RDWR);
  if(fifo_sem == SEM_FAILED) {
    printf("Failed to open semaphore\n");
    exit(1);
  }

  free_sem = sem_open(free_path, O_RDWR);
  if(free_sem == SEM_FAILED) {
    printf("Failed to open semaphore\n");
    exit(1);
  }
}

int take_chair(){
  int barber_state;
  if(sem_getvalue(barber_sem, &barber_state) == -1) {
    printf("Cannot get barber state\n");
    exit(1);
  }

  pid_t pid = getpid();

  if(barber_state == 0) {

    if(sem_post(barber_sem) == -1) {
      printf("Cannot release semaphore\n");
    }

    long time_checkpoint = get_time();
    printf("Time: %ld, Barber has been awakened!\n", time_checkpoint);
    fflush(stdout);

    if(sem_wait(free_sem) == -1) {
      printf("Cannot get semaphore\n");
      exit(1);
    }

    fifo->chair = pid;

    return 1;

  }
  else {
    int res =  push_fifo(fifo, pid);
    if(res == -1) {
      long time_checkpoint = get_time();
      printf("Time: %ld, There's no free chair for customer with id: %d\n", time_checkpoint, pid);
      fflush(stdout);
      return -1;
    }
    else {
      long time_checkpoint = get_time();
      printf("Time: %ld, Customer %d has taken a chair!\n", time_checkpoint, pid);
      fflush(stdout);
      return 0;
    }
  }
}

void get_cut(int cuts_no){
  while(cuts_count < cuts_no) {
    if(sem_wait(barbering_sem) == -1) {
      printf("Cannot get semaphore\n");
      exit(1);
    }

    if(sem_wait(fifo_sem) == -1) {
      printf("Cannot get semaphore\n");
      exit(1);
    }

    int res = take_chair();

    if(sem_post(fifo_sem) == -1) {
      printf("Cannot release semaphore\n");
    }

    if(sem_post(barbering_sem) == -1) {
      printf("Cannot release semaphore\n");
    }

    if(res != -1){
      sigsuspend(&full_mask);
      long time_checkpoint = get_time();
      printf("Time: %ld, Customer %d has been barbered!\n", time_checkpoint, getpid());
      fflush(stdout);
    }
  }
}

int main(int argc, char** argv){

  if(argc < 3) {
    printf("Not enough arguments\n");
    exit(1);
  }

  int customers_no = atoi(argv[1]);
  int cuts_no = atoi(argv[2]);

  if(customers_no < 1 || customers_no > 100){
    printf("Incorrect customers number");
    exit(1);
  }

  if(cuts_no < 1 || cuts_no > 50){
    printf("Incorrect cuts number");
    exit(1);
  }

  if(atexit(exit_handler) == -1) {
    printf("Failed to declare atexit function\n");
    exit(1);
  }

  if(signal(SIGINT, int_handler) == SIG_ERR) {
    printf("Catching SIGINT failed\n");
    exit(1);
  }

  if(signal(SIGRTMIN, rtmin_handler) == SIG_ERR) {
    printf("Catching SIGRTMIN failed\n");
    exit(1);
  }

  prepare_fifo();
  prepare_semaphores();

  if(sigfillset(&full_mask) == -1) {
    printf("Sigfillset failed\n");
    exit(1);
  }
  if(sigdelset(&full_mask, SIGRTMIN) == -1) {
    printf("Failed to remove SIGRTMIN from full_mask\n");
    exit(1);
  }
  if(sigdelset(&full_mask, SIGINT) == -1) {
    printf("Failed to remove SIGINT from full_mask\n");
    exit(1);
  }

  sigset_t mask;

  if(sigemptyset(&mask) == -1) {
    printf("Sigemptyset fail\n");
    exit(1);
  }

  if(sigaddset(&mask, SIGRTMIN) == -1) {
    printf("Sigaddset fail\n");
    exit(1);
  }

  if(sigprocmask(SIG_BLOCK, &mask, NULL) == -1) {
    printf("Sigprocmask fail\n");
    exit(1);
  }

  for(int i=0; i<customers_no; i++){
    pid_t pid = fork();
    if(pid == -1) {
      printf("Failed to fork\n");
      exit(1);
    }

    if(pid == 0){
      //CHILD
      get_cut(cuts_no);
      return 0;
    }
  }
  printf("All customers has been barbered\n");
  fflush(stdout);

  while(1){
    wait(NULL);
    if (errno == ECHILD) break;
  }

  return 0;
}
