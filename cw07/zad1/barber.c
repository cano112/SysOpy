#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <time.h>

#include "fifo.h"
#include "commons.h"

key_t key;
int shm_id = -1;
int sem_id = -1;
FIFO_Q* fifo = NULL;


void int_handler(int signo){
  exit(signo);
}

void exit_handler(void){
  if(shmdt(fifo) == -1) {
    printf("Error detaching shared memory\n");
  }
  else {
    printf("Shared memory detached\n");
  }

  if(shmctl(shm_id, IPC_RMID, NULL) == -1) {
    printf("Error deleting shared memory\n");
  }
  else {
    printf("Shared memory deleted\n");
  }
  if(semctl(sem_id, 0, IPC_RMID) == -1) {
    printf("Error deleting semaphores");
  }
  else {
    printf("Semaphores deleted\n");
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

void prepare_fifo(int chair_num){

  if(chair_num < 1 || chair_num > 100) {
    printf("Wrong number of chairs");
    exit(1);
  }

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

  shm_id = shmget(key, sizeof(FIFO), IPC_CREAT | IPC_EXCL | 0666);
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
  init_fifo(fifo, chair_num);
}

void prepare_semaphores(){
  sem_id = semget(key, 4, IPC_CREAT | IPC_EXCL | 0666);
  if(sem_id == -1) {
    printf("Failed to get semaphore set id\n");
    exit(1);
  }

  if(semctl(sem_id, 0, SETVAL, 0) == -1) {
    printf("Failed to set semaphore\n");
    exit(1);
  }

  for(int i=1; i<3; i++){
    if(semctl(sem_id, i, SETVAL, 1) == -1) {
      printf("Failed to set semaphore\n");
      exit(1);
    }
  }
}

pid_t take_chair(struct sembuf* buf){
  buf->sem_num = FIFO;
  buf->sem_op = -1;
  if(semop(sem_id, buf, 1) == -1) {
    printf("Cannot get semaphore\n");
    exit(1);
  }

  pid_t to_cut = fifo->chair;

  buf->sem_op = 1;
  if(semop(sem_id, buf, 1) == -1) {
    printf("Cannot release semaphore\n");
    exit(1);
  }

  return to_cut;
}

void cut(pid_t pid){
  long time_checkpoint = get_time();
  printf("Time: %ld, Barber is preparing to cut %d\n", time_checkpoint, pid);
  fflush(stdout);

  kill(pid, SIGRTMIN);

  time_checkpoint = get_time();
  printf("Time: %ld, Barber has finished cutting %d\n", time_checkpoint, pid);
  fflush(stdout);
}

void loop(){
  while(1) {
    struct sembuf buf;
    buf.sem_num = BARBER;
    buf.sem_op = -1;
    buf.sem_flg = 0;

    if(semop(sem_id, &buf, 1) == -1) {
      printf("Cannot get semaphore\n");
      exit(1);
    }

    pid_t to_cut = take_chair(&buf);
    cut(to_cut);

    while(1){
      buf.sem_num = FIFO;
      buf.sem_op = -1;
      if(semop(sem_id, &buf, 1) == -1) {
        printf("Cannot get semaphore\n");
        exit(1);
      }
      //write_out_fifo(fifo);
      to_cut = pop_fifo(fifo);

      if(to_cut != -1){
        buf.sem_op = 1;
        if(semop(sem_id, &buf, 1) == -1) {
          printf("Cannot release semaphore\n");
          exit(1);
        }

        cut(to_cut);

      } else {
        long time_checkpoint = get_time();
        printf("Time: %ld, Barber is going to sleep...\n", time_checkpoint);
        fflush(stdout);

        buf.sem_num = BARBER;
        buf.sem_op = -1;
        if(semop(sem_id, &buf, 1) == -1) {
          printf("Cannot get semaphore\n");
          exit(1);
        }

        buf.sem_num = FIFO;
        buf.sem_op = 1;
        if(semop(sem_id, &buf, 1) == -1) {
          printf("Cannot get semaphore\n");
          exit(1);
        }
        break;
      }
    }
  }
}

int main(int argc, char** argv){

  if(atexit(exit_handler) == -1) {
    printf("Failed to declare atexit function\n");
    exit(1);
  }

  if(argc < 2) {
    printf("Not enough arguments\n");
    exit(1);
  }

  if(signal(SIGINT, int_handler) == SIG_ERR) {
    printf("Catching SIGINT failed\n");
    exit(1);
  }

  prepare_fifo(atoi(argv[1]));
  prepare_semaphores();
  loop();

  return 0;
}
