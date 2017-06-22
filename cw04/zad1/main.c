#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

enum mode {
  FORWARD,
  BACKWARD
};

enum mode mode = FORWARD;

void signal_handler(int signum) {
  if(signum == SIGTSTP) {
    mode = (mode+1)%2;

    if(signal(SIGTSTP, signal_handler) == SIG_ERR) {
      perror("Signal error");
    }
  }
  else {
    if(signum == SIGINT) {
      printf("\nOdebrano sygnal SIGINT\n");
      exit(1);
    }
  }

}

void writeOutLetters() {
  char letter = 'A';
  while(1) {

    printf("%c ", letter);
    fflush(stdout);

    if(mode == FORWARD) {
      if(letter == 'Z') letter = 'A';
      else letter++;
    }
    else {
      if(letter == 'A') letter = 'Z';
      else letter--;
    }

    sleep(1);
  }
}

int main(void) {

  if(signal(SIGTSTP, signal_handler) == SIG_ERR) {
    perror("Signal error");
  }

  struct sigaction action;

  action.sa_handler = signal_handler;
  sigemptyset(&action.sa_mask);
  action.sa_flags = 0;

  if(sigaction(SIGINT, &action, NULL) == -1) {
    perror("Signal error");
  }

  writeOutLetters();

  return 0;
}
