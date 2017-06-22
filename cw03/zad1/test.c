#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

int main(int argc, char **argv) {

  if(argc>1) {
    char *env = getenv(argv[1]);
    printf("%s=%s\n", argv[1], env);
  }
  else {
    errno = EINVAL;
    perror("No argument");
  }

  return 0;
}
