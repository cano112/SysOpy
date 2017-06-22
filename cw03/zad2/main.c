#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/time.h>

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

int interpret_instr(char *line, int time_limit, int mem_limit) {
  int ret_val = 0;
  line = strip_string(line, 0);
  char *line_cpy = malloc(sizeof(line));
  strcpy(line_cpy, line);

  char *argv[10];
  int i = 0;
  while((argv[i] = strsep(&line, " ")) != NULL && i < 9) {
    i++;
  }
  argv[++i] = NULL;
  pid_t pid = fork();

  if(pid == 0) {
    //CHILD

    struct rlimit new_time_limit;
    struct rlimit new_mem_limit;

    new_time_limit.rlim_cur = time_limit; // sec
    new_time_limit.rlim_max = time_limit;

    new_mem_limit.rlim_cur = mem_limit*1048576; // MB to B
    new_mem_limit.rlim_max = mem_limit*1048576;

    if(setrlimit(RLIMIT_CPU, &new_time_limit) != 0 || setrlimit(RLIMIT_MEMLOCK, &new_mem_limit) != 0) {
      ret_val = 1;
    }
    else {
      if(execvp(*argv, argv) == -1) {
        ret_val = 1;
      }
    }

  }
  else {
    if(pid > 0) {
      //PARENT
      int wstatus;

      struct rusage *usage = malloc(sizeof(struct rusage));

      getrusage(RUSAGE_CHILDREN, usage);
      struct timeval user0 = usage->ru_utime;
    	struct timeval sys0 = usage->ru_stime;

      waitpid(pid, &wstatus, 0);

      getrusage(RUSAGE_CHILDREN, usage);
      struct timeval user1 = usage->ru_utime;
      struct timeval sys1 = usage->ru_stime;

      printf("Stats (%s):\nuser: %ld\nsystem: %ld\n\n",
        line_cpy,
      	(user1.tv_sec-user0.tv_sec)*1000000+user1.tv_usec-user0.tv_usec,
      	(sys1.tv_sec-sys0.tv_sec)*1000000+sys1.tv_usec-sys0.tv_usec);

      if(WIFEXITED(wstatus) != 1) ret_val = 1;

      free(line_cpy);
      free(usage);
    }
    else {
      //FORK FAILED
      ret_val = 1;
    }
  }

  return ret_val;
}

int interpret_line(char *line, int time_limit, int mem_limit) {
  int status = 0;

  switch(line[0]) {
    case '#': //Environmental variables operations
      status = interpret_env_op(line);
      break;
    case '\n':  //New line
      break;
    case '%': //Comment
      break;
    default:  //Instruction
      status = interpret_instr(line, time_limit, mem_limit);
  }
  return status;
}

int main(int argc, char **argv) {

  FILE *batch_file;

  if(argc > 3) {
      if((batch_file = fopen(argv[1], "r"))) {
        char *line;
        int count = 1;

        while ((line = get_next_line(batch_file)) != NULL) {
          if(interpret_line(line, atoi(argv[2]), atoi(argv[3])) != 0) {
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
