#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <time.h>

static int signals_received_child = 0;
static int signals_received_parent = 0;
int signals_sent = 0;
static pid_t ppid;
static pid_t pid;

int send_signal(pid_t pid, int sig, int mode) {
  if(mode==1) {
    if(kill(pid, sig) == -1) {
      return -1;
    }
    return 0;
  }
  else {
    if(mode==2) {
      union sigval val;
      if(sigqueue(pid, sig, val) == -1){
        return -1;
      }
      return 0;
    }
    errno = EINVAL;
    return -1;
  }
}

void sig_handler_child(int signum) {
  if(signum == SIGUSR1 || signum == SIGRTMIN) {
    signals_received_child++;
    printf("Signals received (child): %d\n", signals_received_child);
  }
  else {
    if(signum == SIGRTMAX || signum == SIGUSR2) {
      printf("Child exited\n");
      exit(0);
    }
  }
}

void sig_handler_parent(int signum) {
  if(signum == SIGUSR1 || signum == SIGRTMIN) {
    signals_received_parent++;
    printf("Signals received (parent): %d\n", signals_received_parent);
  }
  else {
    if (signum == SIGCHLD) {
      exit(0);
    }
    else {
      if(signum == SIGINT) {
        kill(pid, SIGTERM);
        exit(1);
      }
    }
  }
}

int main(int argc, char **argv) {

  if(argc < 3) {
      errno = EINVAL;
      perror("Not enough arguments");
  }

  const int L = atoi(argv[1]);
  const int TYPE = atoi(argv[2]);
  ppid = getpid();
  pid = fork();

  if(pid == 0) {
    /*CHILD*/

    struct sigaction action;
    action.sa_handler = sig_handler_child;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;

    sigaction(SIGUSR1, &action, NULL);
    sigaction(SIGRTMIN, &action, NULL);
    sigaction(SIGUSR2, &action, NULL);
    sigaction(SIGRTMAX, &action, NULL);

    sigset_t mask, oldmask;
    sigemptyset (&mask);
    sigaddset (&mask, SIGUSR1);

    sigprocmask (SIG_BLOCK, &mask, &oldmask);
    while (signals_received_child < L)
      sigsuspend (&oldmask);
    sigprocmask (SIG_UNBLOCK, &mask, NULL);

    for(int i = 0; i < signals_received_child; i++) {
      sleep(1);

      if(TYPE == 3) {
        if(send_signal(ppid, SIGRTMIN, 1) == -1) {
          perror("Cannot send signal");
          exit(1);
        }
      }
      else {
        if(send_signal(ppid, SIGUSR1, TYPE) == -1) {
          perror("Cannot send signal");
          exit(1);
        }
      }
    }

    exit(0);

  }
  else {
    /*PARENT*/

    if(pid > 0) {
      for(int i = 0; i < L; i++) {
        sleep(1);
        if(TYPE == 3) {
          if(send_signal(pid, SIGRTMIN, 1) == -1) {
            perror("Cannot send signal");
            exit(1);
          }
        }
        else {
          if(send_signal(pid, SIGUSR1, TYPE) == -1) {
            perror("Cannot send signal");
            exit(1);
          }
        }
        signals_sent++;
        printf("Signals sent: %d\n", signals_sent);
      }

      struct sigaction action;
      action.sa_handler = sig_handler_parent;
      sigemptyset(&action.sa_mask);
      action.sa_flags = 0;
      sigaction(SIGUSR1, &action, NULL);
      sigaction(SIGRTMIN, &action, NULL);
      sigaction(SIGINT, &action, NULL);
      sigaction(SIGCHLD, &action, NULL);

      while(signals_received_parent < L) {
        sleep(1);
      }

      if(TYPE == 3) {
        if(send_signal(pid, SIGRTMAX, 1) == -1) {
          perror("Cannot send signal");
          exit(1);
        }
      }
      else {
        if(send_signal(pid, SIGUSR2, TYPE) == -1) {
          perror("Cannot send signal");
          exit(1);
        }
      }


      sigset_t mask, oldmask;
      sigemptyset (&mask);
      sigaddset (&mask, SIGINT);
      sigaddset (&mask, SIGCHLD);

      sigprocmask (SIG_BLOCK, &mask, &oldmask);
      while (1)
        sigsuspend (&oldmask);
      sigprocmask (SIG_UNBLOCK, &mask, NULL);
    }
    else {
      perror("Fork error");
    }
  }

  return 0;
}
