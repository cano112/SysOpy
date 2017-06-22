#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/wait.h>

#include "fifo.h"
#include "commons.h"

key_t key;
int shm_id = -1;
int sem_id = -1;
FIFO_Q* fifo = NULL;
volatile int cuts_count = 0;
sigset_t full_mask;

void int_handler(int signo){
  exit(signo);
}

void rtmin_handler(int signo){
  cuts_count++;
}

void exit_handler(void){
  if(shmdt(fifo) == -1) {
    printf("Error detaching shared memory\n");
  }
  else {
    printf("Shared memory detached\n");
  }
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

  char* path = getenv(PATH);
  if(path == NULL) {
    printf("Failed to get env variable\n");
    exit(1);
  }

  key = ftok(path, ID);
  if(key == -1) {
      printf("Failed to get key\n");
      exit(1);
  }

  shm_id = shmget(key, 0, 0);
  if(shm_id == -1) {
    printf("Failed to get shared memory id\n");
    exit(1);
  }

  void* shm_ptr = shmat(shm_id, NULL, 0);
  if(shm_ptr == (void*)(-1)) {
    printf("Failed to get shared memory pointer\n");
    exit(1);
  }

  fifo = (FIFO_Q*)shm_ptr;
}

void prepare_semaphores() {
  sem_id = semget(key, 0, 0);
  if(sem_id == -1) {
    printf("Failed to get semaphore set id\n");
    exit(1);
  }
}

int take_chair(){
  int barber_state = semctl(sem_id, 0, GETVAL);
  if(barber_state == -1) {
    printf("Cannot get barber state\n");
    exit(1);
  }

  pid_t pid = getpid();

  if(barber_state == 0){ //barber sleeping
    struct sembuf buf;
    buf.sem_num = BARBER;
    buf.sem_op = 1;
    buf.sem_flg = 0;

    if(semop(sem_id, &buf, 1) == -1) {
      printf("Cannot release semaphore\n");
      exit(1);
    }

    long time_checkpoint = get_time();
    printf("Time: %ld, Barber has been awakened!\n", time_checkpoint);
    fflush(stdout);
    if(semop(sem_id, &buf, 1) == -1) {
      printf("Cannot release semaphore\n");
      exit(1);
    }

    fifo->chair = pid;

    return 1;

  } else {
    int res =  push_fifo(fifo, pid);
    if(res == -1){
      long time_checkpoint = get_time();
      printf("Time: %ld, There's no free chair for customer with id: %d\n", time_checkpoint, pid);
      fflush(stdout);
      return -1;

    } else {
      long time_checkpoint = get_time();
      printf("Time: %ld, Customer %d has taken a chair!\n", time_checkpoint, pid);
      fflush(stdout);
      return 0;
    }
  }
}

void get_cut(int cuts_no){
  while(cuts_count < cuts_no){
    struct sembuf buf;
    buf.sem_num = BARBERING;
    buf.sem_op = -1;
    buf.sem_flg = 0;

    if(semop(sem_id, &buf, 1) == -1) {
      printf("Cannot get semaphore\n");
      exit(1);
    }

    buf.sem_num = FIFO;
    if(semop(sem_id, &buf, 1) == -1) {
      printf("Cannot get semaphore\n");
      exit(1);
    }

    int res = take_chair();

    buf.sem_op = 1;
    if(semop(sem_id, &buf, 1) == -1) {
      printf("Cannot release semaphore\n");
      exit(1);
    }

    buf.sem_num = BARBERING;
    if(semop(sem_id, &buf, 1) == -1) {
      printf("Cannot release semaphore\n");
      exit(1);
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
