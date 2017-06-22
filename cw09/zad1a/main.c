//UPRZYWILEJOWANIE PISARZY

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <string.h>
#include <semaphore.h>

#define ARRAY_SIZE 10
#define VAL_MIN 0
#define VAL_MAX 100

int ext_out = 0;

int array[ARRAY_SIZE] = {0};

sem_t access_sem;
sem_t readers_sem;
sem_t order_sem;

int readers = 0;

int rand_int(int from, int to) {
  return rand()%(to-from+1)+from;
}

void array_randomize(int *array, int from_index, int to_index, int min, int max) {
  for(int i = from_index; i <= to_index; i++) {
    array[i] = rand_int(min, max);
  }
}

void modify_array_rand(int *array, int array_size, int min, int max) {
  int n = rand_int(1, array_size);
  pid_t tid = syscall(SYS_gettid);

  int *positions = (int*)malloc(n*sizeof(int));
  for(int i = 0; i<n; i++) {
    while(1) {
      int pos = rand_int(0, array_size-1);

      int j;
      for(j = 0; j < i; j++) {
        if(positions[j] == pos) break;
      }
      if(i == j) {
        positions[i] = pos;
        break;
      }
    }
  }

  for(int i = 0; i < n; i++) {
    int val = rand_int(min, max);
    array[positions[i]] = val;
    if(ext_out) printf("WRITER TID %d: array[%d] := %d\n", tid,  positions[i], val);
  }
  printf("WRITER TID %d: Array modified\n", tid);

  free(positions);
}

void find_divisible(int *array, int array_size, int n) {
  int count = 0;
  pid_t tid = syscall(SYS_gettid);

  for(int i = 0; i<array_size; i++) {
    if(array[i]%n == 0) {
      if(ext_out) printf("READER TID %d: array[%d] := %d is divisible by %d\n", tid,  i, array[i], n);
      count++;
    }
  }
  printf("READER TID %d: total count of numbers divisible by %d: %d\n", tid, n, count);
}

void array_write_out(int *array, int from_index, int to_index) {
  for(int i = from_index; i <= to_index; i++) {
    printf("%d ", array[i]);
  }
  printf("\n");
}

void *reader_handler(void *arg) {
  while(1) {
      sem_wait(&order_sem);
      sem_wait(&readers_sem);
      if(readers == 0) {
        sem_wait(&access_sem);
      }
      readers++;
      sem_post(&order_sem);
      sem_post(&readers_sem);

      find_divisible(array, ARRAY_SIZE, 5);

      sem_wait(&readers_sem);
      readers--;
      if(readers == 0) {
        sem_post(&access_sem);
      }
      sem_post(&readers_sem);
      //sleep(1);
  }
}

void *writer_handler(void *arg) {
  while(1) {

    sem_wait(&order_sem);
    sem_wait(&access_sem);
    sem_post(&order_sem);

    modify_array_rand(array, ARRAY_SIZE, VAL_MIN, VAL_MAX);

    sem_post(&access_sem);
    //sleep(1);
  }
}

int main(int argc, char **argv) {

  if (argc < 3) {
    errno = EINVAL;
    perror("Wrong arguments");
    exit(1);
  }

  if(argc > 3 && strcmp(argv[3], "-i") == 0) {
    ext_out = 1;
  }
  const int READERS_NUMBER = atoi(argv[1]);
  const int WRITERS_NUMBER = atoi(argv[2]);

  if(READERS_NUMBER > 100 || WRITERS_NUMBER > 100 || READERS_NUMBER < 1 || WRITERS_NUMBER < 1) {
    errno = EINVAL;
    perror("Wrong arguments");
    exit(1);
  }

  srand(time(NULL));

  array_randomize(array, 0, ARRAY_SIZE-1, VAL_MIN, VAL_MAX);
  array_write_out(array, 0, ARRAY_SIZE-1);

  sem_init(&access_sem, 0, 1);
  sem_init(&readers_sem, 0, 1);
  sem_init(&order_sem, 0, 1);

  pthread_attr_t attr;
  if(pthread_attr_init(&attr) != 0) {
    perror("Cannot init attr");
    exit(1);
  }

  if(pthread_attr_setschedpolicy(&attr, SCHED_FIFO) != 0) {
    perror("Cannot set sched policy");
    exit(1);
  }

  if(pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED) != 0) {
    perror("Cannot set inherit sched policy");
    exit(1);
  }

  struct sched_param sched_param;
  sched_param.__sched_priority = sched_get_priority_min(SCHED_FIFO);

  if(pthread_attr_setschedparam(&attr, &sched_param)) {
    perror("Cannot set sched param");
    exit(1);
  }

  pthread_t *readers = (pthread_t*)malloc(READERS_NUMBER*sizeof(pthread_t));
  pthread_t *writers = (pthread_t*)malloc(WRITERS_NUMBER*sizeof(pthread_t));

  for(int i = 0; i < READERS_NUMBER; i++) {
    errno = pthread_create(&readers[i], &attr, reader_handler, NULL);
    if(errno != 0) {
      if(errno == EPERM) {
        if(pthread_create(&readers[i], NULL, reader_handler, NULL) != 0) {
          perror("Cannot create reader's thread");
          exit(1);
        }
      }
      else {
        perror("Cannot create reader's thread");
        exit(1);
      }
    }
  }

  for(int i = 0; i < WRITERS_NUMBER; i++) {
    errno = pthread_create(&writers[i], &attr, writer_handler, NULL);
    if(errno != 0) {
      if(errno == EPERM) {
        if(pthread_create(&writers[i], NULL, writer_handler, NULL) != 0) {
          perror("Cannot create writer's thread");
          exit(1);
        }
      }
      else {
        perror("Cannot create writer's thread");
        exit(1);
      }
    }
  }

  for(int i = 0; i < READERS_NUMBER; i++) {
    if(pthread_join(readers[i], NULL) != 0) {
      perror("Cannot join reader's thread");
      exit(1);
    }
  }

  for(int i = 0; i < WRITERS_NUMBER; i++) {
    if(pthread_join(writers[i], NULL) != 0) {
      perror("Cannot join writers's thread");
      exit(1);
    }
  }

  if(pthread_attr_destroy(&attr) !=0) {
    perror("Cannot destory attr");
    exit(1);
  }

  free(readers);
  free(writers);

  return 0;
}
