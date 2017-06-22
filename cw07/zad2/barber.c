#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <ctype.h>
#include <time.h>
#include <signal.h>
#include <sys/types.h>

#include <sys/mman.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "fifo.h"
#include "commons.h"

FIFO_Q* fifo = NULL;

sem_t* barber_sem;
sem_t* barbering_sem;
sem_t* fifo_sem;
sem_t* free_sem;

void int_handler(int signo){
  exit(signo);
}

long get_time(){
  struct timespec checkpoint;
  if(clock_gettime(CLOCK_MONOTONIC, &checkpoint) == -1) {
    printf("Failed to get time");
    exit(1);
  }
  return checkpoint.tv_nsec / 1000;
}

void exit_handler(void) {

  if(munmap(fifo, sizeof(fifo)) == -1) printf("Error detaching shared memory\n");
  else printf("Shared memory detached\n");

  if(shm_unlink(shm_path) == -1) printf("Error deleting shared memory\n");
  else printf("Shared memory deleted\n");

  if(sem_close(barber_sem) == -1) printf("Error closing semaphore");
  else printf("Semaphore closed\n");

  if(sem_unlink(barber_path) == -1) printf("Error deleting semaphore");
  else printf("Semaphore deleted\n");

  if(sem_close(barbering_sem) == -1) printf("Error closing semaphore");
  else printf("Semaphore closed\n");

  if(sem_unlink(barbering_path) == -1) printf("Error deleting semaphore");
  else printf("Semaphore deleted\n");

  if(sem_close(fifo_sem) == -1) printf("Error closing semaphore");
  else printf("Semaphore closed\n");

  if(sem_unlink(fifo_path) == -1) printf("Error deleting semaphore");
  else printf("Semaphore deleted\n");

  if(sem_close(free_sem) == -1) printf("Error closing semaphore");
  else printf("Semaphore closed\n");

  if(sem_unlink(free_path) == -1) printf("Error deleting semaphore");
  else printf("Semaphore deleted\n");
}

void prepare_fifo(int chair_num){

  if(chair_num < 1 || chair_num > 100) {
    printf("Wrong number of chairs");
    exit(1);
  }

  int shm_id = shm_open(shm_path, O_RDWR | O_CREAT | O_EXCL, 0666);
  if(shm_id == -1) {
    printf("Failed to get shared memory id\n");
    exit(1);
  }

  if(ftruncate(shm_id, sizeof(FIFO_Q)) == -1) {
    printf("Failed to truncate shared memory\n");
    exit(1);
  }

  void* shm_ptr = mmap(NULL, sizeof(FIFO_Q), PROT_READ | PROT_WRITE, MAP_SHARED, shm_id, 0);

  if(shm_ptr == (void*)(-1)) {
    printf("Failed to get shared memory pointer\n");
    exit(1);
  }

  fifo = (FIFO_Q*)shm_ptr;

  init_fifo(fifo, chair_num);
}

void prepare_semaphores() {

  barber_sem = sem_open(barber_path, O_CREAT | O_EXCL | O_RDWR, 0666, 0);
  if(barber_sem == SEM_FAILED) {
    printf("Failed to open semaphore\n");
    exit(1);
  }

  barbering_sem = sem_open(barbering_path, O_CREAT | O_EXCL | O_RDWR, 0666, 1);
  if(barber_sem == SEM_FAILED) {
    printf("Failed to open semaphore\n");
    exit(1);
  }

  fifo_sem = sem_open(fifo_path, O_CREAT | O_EXCL | O_RDWR, 0666, 1);
  if(barber_sem == SEM_FAILED) {
    printf("Failed to open semaphore\n");
    exit(1);
  }

  free_sem = sem_open(free_path, O_CREAT | O_EXCL | O_RDWR, 0666, 0);
  if(barber_sem == SEM_FAILED) {
    printf("Failed to open semaphore\n");
    exit(1);
  }
}

pid_t take_chair() {
  if(sem_wait(fifo_sem) == -1) {
    printf("Cannot get semaphore\n");
    exit(1);
  }

  pid_t to_cut = fifo->chair;

  if(sem_post(fifo_sem) == -1) {
    printf("Cannot release semaphore\n");
  }

  return to_cut;
}

void cut(pid_t pid) {
  long time_checkpoint = get_time();
  printf("Time: %ld, Barber is preparing to cut %d\n", time_checkpoint, pid);
  fflush(stdout);

  kill(pid, SIGRTMIN);

  time_checkpoint = get_time();
  printf("Time: %ld, Barber has finished cutting %d\n", time_checkpoint, pid);
  fflush(stdout);
}

void loop() {

  while(1) {
    if(sem_wait(barber_sem) == -1) {
      printf("Cannot get semaphore\n");
      exit(1);
    }

    if(sem_post(barber_sem) == -1) {
      printf("Cannot release semaphore\n");
    }

    if(sem_post(free_sem) == -1) {
      printf("Cannot release semaphore\n");
    }

    pid_t to_cut = take_chair();
    cut(to_cut);

    while(1) {
      if(sem_wait(fifo_sem) == -1) {
        printf("Cannot get semaphore\n");
        exit(1);
      }
      to_cut = pop_fifo(fifo);

      if(to_cut != -1) {
        if(sem_post(fifo_sem) == -1) {
          printf("Cannot release semaphore\n");
        }
        cut(to_cut);

      }
      else {
        long time_checkpoint = get_time();
        printf("Time: %ld, Barber is going to sleep...\n", time_checkpoint);
        fflush(stdout);

        if(sem_wait(barber_sem) == -1) {
          printf("Cannot get semaphore\n");
          exit(1);
        }

        if(sem_post(fifo_sem) == -1) {
          printf("Cannot release semaphore\n");
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
