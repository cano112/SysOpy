#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#define MAX_BUF 1024
struct complex_t {
  double re;
  double im;
};

struct complex_t get_random_complex(double min_re, double max_re, double min_im, double max_im) {
  struct complex_t res;
  res.re = (double)rand()/(double)(RAND_MAX/(max_re-min_re))+min_re;
  res.im = (double)rand()/(double)(RAND_MAX/(max_im-min_im))+min_im;

  return res;
}

void complex_square(struct complex_t *c) {
  double tmp = c->re;
  c->re = (c->re*c->re) - (c->im*c->im);
  c->im = 2 * tmp * c->im;
}

unsigned int iters(struct complex_t c, const unsigned int K) {

  struct complex_t z;
  z.re = 0;
  z.im = 0;
  unsigned int i = 0;
  for(i = 1; i <= K; i++) {
    complex_square(&z);
    z.re = z.re + c.re;
    z.im = z.im + c.im;
    if(z.re*z.re+z.im*z.im > 4) break;
  }
  return i-1;
}

int main(int argc, char **argv) {
  srand(time(NULL));

  if(argc < 4) {
    errno = EINVAL;
    perror("Not enough arguments");
    exit(1);
  }

  const unsigned int K = atoi(argv[3]);
  const unsigned int N = atoi(argv[2]);

  int fd;
  if((fd = open(argv[1], O_WRONLY)) == -1) {
    perror("Error opening pipe");
    exit(1);
  }

  for(unsigned int i = 0; i<N;i++) {
    struct complex_t com = get_random_complex(-2.0, 1.0, -1.0, 1.0);
    unsigned int it = iters(com, K);

    char buffer[MAX_BUF] = {0x0};
    sprintf(buffer, "%.16lf,%.16lf,%u,\n", com.re, com.im, it);

    if(write(fd, buffer, MAX_BUF) == -1) {
      perror("Error writing to pipe");
    }
  }
  close(fd);

  return 0;
}
