#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

char *strip_string(char *input, int mode) {
  int len = strlen(input);
  if(len > 0 && mode == 1) {
    input++;
  }

  if(len > 1) {
    if(mode == 1) {
      input[len - 2] = '\0';
    }
    else {
      input[len - 1] = '\0';
    }
  }

  return input;
}

char *get_next_line(FILE *handler) {

  char *line;
  size_t len = 0;
  ssize_t nread;
  if((nread = getline(&line, &len, handler)) == -1) {
    return NULL;
  }
  return line;
}

int interpret_env_op(char *line) {

  line = strip_string(line, 1);

  char *name = NULL;
  char *value = NULL;

  name = strsep(&line, " ");
  value = strsep(&line, " ");
  int res;
  if(value == NULL) {
    res = unsetenv(name);
  }
  else {
    res = setenv(name, value, 1);
  }

  return res;
}

int interpret_instr(char *line) {
  line = strip_string(line, 0);
  char *argv[10];
  int i = 0;
  while((argv[i] = strsep(&line, " ")) != NULL && i < 9) {
    i++;
  }
  argv[++i] = NULL;

  pid_t pid = fork();

  if(pid == 0) {
    //CHILD
    if(execvp(*argv, argv) == -1) {
      return 1;
    }
  }
  else {
    if(pid > 0) {
      //PARENT
      int wstatus;
      waitpid(pid, &wstatus, 0);
      if(WIFEXITED(wstatus) != 1) return 1;

      return 0;
    }
    else {
      //FORK FAILED
      return 1;
    }
  }

  return 0;
}
int interpret_line(char *line) {
  int status;

  switch(line[0]) {
    case '#': //Environmental variables operations
      status = interpret_env_op(line);
      break;
    case '\n': //New line
      break;
    case '%': //Comment
      break;
    default:  //Instruction
      status = interpret_instr(line);
  }
  return status;
}

int main(int argc, char **argv) {

  FILE *batch_file;

  if(argc > 1) {
      if((batch_file = fopen(argv[1], "r"))) {
        char *line;
        int count = 1;
        while ((line = get_next_line(batch_file)) != NULL) {
          if(interpret_line(line) != 0) {
            perror("Cannot execute command");
            fprintf(stderr, "Error executing %d line\n%s\n\n", count, line);
          }
          count++;
        }
      }
      else {
        perror("Error opening file");
      }
  }
  else {
    errno = EINVAL;
    perror("No argument");
  }
  fclose(batch_file);


  return 0;
}
