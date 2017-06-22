#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <string.h>
#include <signal.h>

#define RECORD_SIZE 1024

void signal_handler(int signum) {
  pid_t pid = getpid();
  pid_t tid = syscall(SYS_gettid);
  //printf("Signal received, PID: %d, TID: %d\n", pid, tid);
}
typedef struct thread_param {
  FILE *file;
  int n_records;
  int n_threads;
  char *keyword;
  pthread_t *thread_ids;
} thread_param;

typedef struct record {
  int id;
  char *text;
} record;

typedef struct buffers_s {
  int n_records;
  record *buffers;
} buffers_s;


pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
volatile long int bytes_read = 0;
long int file_length = 0;

void free_mem(void *mem) {
  for(int i = 0; i<(((buffers_s*)mem)->n_records); i++) {
    free(((buffers_s*)mem)->buffers[i].text);
  }
  free(((buffers_s*)mem)->buffers);
}
void *thread_handler(void *arg) {

  // sigset_t mask;
  // sigemptyset(&mask);
  // sigaddset(&mask, SIGUSR1);
  // //sigaddset(&mask, SIGSTOP);
  // sigprocmask(SIG_BLOCK, &mask, NULL);

  //signal(SIGUSR1, signal_handler);

  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,  NULL);
  pid_t tid = syscall(SYS_gettid);

  pthread_t thread_id = pthread_self();
  thread_param *param = (thread_param*)arg;

  buffers_s buffers_s;
  buffers_s.buffers = (record*)malloc(param->n_records*sizeof(record));
  for(int i = 0; i<param->n_records; i++) {
    buffers_s.buffers[i].text = (char*)malloc((RECORD_SIZE-sizeof(int)+1)*sizeof(char));
  }
  pthread_cleanup_push(free_mem, &buffers_s);
  while(1) {
    if(bytes_read >= file_length) {
      param->file = NULL;
      break;
    }

    int i = 0;
    while(i++ < param->n_records) {

      pthread_mutex_lock(&lock);
      if(!param->file) {
        pthread_mutex_unlock(&lock);
        break;
      }
      if(bytes_read >= file_length) {
        param->file = NULL;
        pthread_mutex_unlock(&lock);
        break;
      }

      fread(&(buffers_s.buffers[i-1].id), sizeof(int), 1, param->file);
      fread(buffers_s.buffers[i-1].text, sizeof(char), RECORD_SIZE-sizeof(int), param->file);
      bytes_read += RECORD_SIZE;
      pthread_mutex_unlock(&lock);
      buffers_s.buffers[i-1].text[RECORD_SIZE] = '\0';
    }

    for(int i = 0; i<param->n_records; i++) {
      if(strstr(buffers_s.buffers[i].text, param->keyword) != NULL) {
        printf("TID: %d, record id: %d\n", tid, buffers_s.buffers[i].id);
      }
    }
  }
  //sleep(100);
  pthread_cleanup_pop(1);
  return NULL;

}

int main(int argc, char **argv) {

  pid_t tid = syscall(SYS_gettid);
  signal(SIGFPE, signal_handler);
  const int N_THREADS = atoi(argv[1]);
  const char *FILE_NAME = argv[2];

  if(argc < 5) {
    errno = EINVAL;
    perror("Not enough arguments");
  }

  FILE *file = fopen(FILE_NAME, "rb");

  if(file == NULL) {
    perror("Cannot open a file with given name");
  }

  fseek(file, 0, SEEK_END);
  file_length = ftell(file);
  fseek(file, 0, SEEK_SET);

  pthread_t *thread_ids = (pthread_t*)calloc(N_THREADS, sizeof(pthread_t));

  thread_param param;
  param.file = file;
  param.n_records = atoi(argv[3]);
  param.keyword = argv[4];
  param.thread_ids = thread_ids;
  param.n_threads = N_THREADS;

  for(int i = 0; i < N_THREADS; i++) {
    if(pthread_create(&thread_ids[i], NULL, &thread_handler, &param) != 0 ) {
      perror("Error creating thread");
    }
  }
  int i = 1/0;
  //pthread_kill(thread_ids[1], SIGUSR1);
  //sleep(100);

  for(int i = 0; i < N_THREADS; i++) {
    if(pthread_join(thread_ids[i], NULL) != 0) {
      perror("Error joining threads");
    }
  }

  fclose(file);


  return 0;
}
