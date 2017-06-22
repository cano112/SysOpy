#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>


struct cmds_t {
  char ***cmds;
  int *argcs;
  int argc;
};

struct cmds_t parse_cmd_line(char *line) {

  char *cmds[30];
  char *token;
  int i = 0;

  while ((token = strsep(&line, "|"))) {

    cmds[i] = token;
    i++;

    if(i == 30) break;
  }
  cmds[i-1][strlen(cmds[i-1])-1] = '\0';

  struct cmds_t parsed;
  parsed.cmds = malloc(30*sizeof(char**));
  parsed.argcs = malloc(30*sizeof(int));
  parsed.argc = i;

  for(int j = 0; j < parsed.argc; j++) {
    char *arg;
    parsed.cmds[j] = malloc(4*sizeof(char*));
    int k = 0;
    while ((arg = strsep(&cmds[j], " "))) {
      if(arg[0] != '\0') {
        parsed.cmds[j][k] = arg;
        k++;
        if(k == 4) break;
      }
    }
    parsed.argcs[j] = k;
  }

  return parsed;
}
void execute_line(char *line) {


  struct cmds_t cmds = parse_cmd_line(line);

  if(cmds.argc == 1) {
    pid_t pid = fork();
    if(pid == 0) {
      execvp(cmds.cmds[0][0], cmds.cmds[0]);
    }
    else {
      if(pid < 0) {
        perror("fork error");
        exit(1);
      }
      wait(NULL);
      return;
    }
  }

  pid_t *pids = malloc(cmds.argc*sizeof(pid_t));
  int **fds = malloc((cmds.argc-1)*sizeof(int*));

  for(int i = 0; i < cmds.argc-1; i++) {
    fds[i] = malloc(2*sizeof(int));
    pipe(fds[i]);
  }
  for(int i = 0; i < cmds.argc; i++) {
    if((pids[i] = fork()) < 0) {
      perror("fork error");
      exit(1);
    }
    else {
      if(pids[i] == 0) {
        //CHILD
        //close all pipes except my
        for(int j = 0; j<cmds.argc-1; j++) {
          if(j == i) {
            close(fds[j][0]);
          }
          else {
            if(j == i-1) {
              close(fds[j][1]);
            }
            else {
              close(fds[j][0]);
              close(fds[j][1]);
            }
          }
        }

        if(i == 0) {
          dup2(fds[i][1], STDOUT_FILENO);
          close(fds[i][1]);

        }
        else {
          if(i == cmds.argc-1) {
            dup2(fds[i-1][0], STDIN_FILENO);
            close(fds[i-1][0]);
          }
          else {
            dup2(fds[i][1], STDOUT_FILENO);
            close(fds[i][1]);
            dup2(fds[i-1][0], STDIN_FILENO);
            close(fds[i-1][0]);
          }
        }
        if(execvp(cmds.cmds[i][0], cmds.cmds[i]) < 0) {
          perror("Cannot execute command");
          exit(1);
        }
        exit(0);
      }
    }
  }

  //PARENT
  for(int i = 0; i<cmds.argc-1; i++) {
    close(fds[i][0]);
    close(fds[i][1]);
  }
  for(int i = 0; i<cmds.argc; i++) {
    wait(NULL);
  }
  return;
}

int main(void) {

  char line[2048];
  while(1) {
    if (fgets(line, 2048, stdin) != NULL) {
      execute_line(line);
    }
  }

  return 0;
}
