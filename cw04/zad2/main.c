#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include "stack.h"
#include <time.h>

#define randnum(min, max) ((rand() % (int)(((max) + 1) - (min))) + (min))

static int signals_no = 0;
static struct PID_Stack stack = { .top = -1 };
static int allowed = 0;
static int processes_done = 0;
static struct PID_Stack all = { .top = -1};

void sig_handler(int signum, siginfo_t *info, __attribute__((unused)) void* context) {
    if(signum == SIGUSR1) {
      printf("Signal %d from PID %d received\n",info->si_signo, info->si_pid);
      signals_no++;
      if(stack_push(&stack, info->si_pid) == -1) {
        perror("Stack full");
      }

      if(signals_no >= info->si_int) {
        int pid;
        while(!stack_is_empty(&stack)) {
          pid = stack_pop(&stack);
          printf("Signal USR2 to PID: %d sent\n", pid);

          kill(pid, SIGUSR2);
        }
      }
    }
    else {
      if(signum == SIGUSR2) {
        allowed = 1;
        printf("Process PID %d received signal USR2\n", getpid());
      }
      else {
        if(signum >= SIGRTMIN && signum <= SIGRTMAX) {
          printf("RT signal %d from PID %d received\n", info->si_signo, info->si_pid);
        }
        else {
          if(signum == SIGCHLD) {
            printf("Child process PID %d terminated with code: %d\n",info->si_pid, info->si_status);
            processes_done++;
          }
          else {
            while(!stack_is_empty(&all))
              kill(stack_pop(&all), SIGTERM);
            exit(1);
          }

        }
      }
    }
}

int main(int argc, char **argv) {

  if(argc > 2) {


    const int N = atoi(argv[1]);
    const int K = atoi(argv[2]);

    pid_t pid;
    pid_t parent_pid = getpid();

    for(int i = 0; i<N; i++) {
      pid = fork();
      if(pid == 0) {
        /* CHILD PROCESS */

        sleep(3+i);
        srand(getpid());

        struct timespec start, end;

        union sigval val;
        val.sival_int = K;
        clock_gettime(CLOCK_REALTIME, &start);
        if(sigqueue(parent_pid, SIGUSR1, val) == -1) {
          perror("Cannot send signal");
          exit(1);
        }

        struct sigaction action;
        action.sa_sigaction = sig_handler;
        sigemptyset(&action.sa_mask);
        action.sa_flags = SA_SIGINFO;
        sigaction(SIGUSR2, &action, NULL);
        sigaction(SIGINT, &action, NULL);

        sigset_t mask, oldmask;
        sigemptyset (&mask);
        sigaddset (&mask, SIGUSR2);

        sigprocmask (SIG_BLOCK, &mask, &oldmask);
        while (allowed == 0)
          sigsuspend (&oldmask);
        sigprocmask (SIG_UNBLOCK, &mask, NULL);
        
        kill(parent_pid, randnum(SIGRTMIN, SIGRTMAX));
        clock_gettime(CLOCK_REALTIME, &end);

        int res = (end.tv_sec-start.tv_sec)*1000000+(end.tv_nsec-start.tv_nsec)/1000;
        exit(res);
      }
      else {
        if(pid > 0) {
          /* PARENT PROCESS */
          stack_push(&all, pid);
          struct sigaction action;
          action.sa_sigaction = sig_handler;
          sigemptyset(&action.sa_mask);
          action.sa_flags = SA_SIGINFO;

          sigaction(SIGUSR1, &action, NULL);
          for(int j = 0;SIGRTMIN+j<=SIGRTMAX;j++) {
            sigaction(SIGRTMIN+j, &action, NULL);
          }
          sigaction(SIGCHLD, &action, NULL);
          sigaction(SIGINT, &action, NULL);

        }
        else {
          perror("Fork error");
          exit(1);
        }
      }
    }
    while(processes_done < N);
  }
  else {
    errno = EINVAL;
    perror("Wrong arguments");
  }


  return 0;
}
