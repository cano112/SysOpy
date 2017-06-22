#include <stdio.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#define MAX_BUF 1024
struct coords_t {
  int x;
  int y;
};

struct point_t {
  double re;
  double im;
  long i;
};
struct coords_t get_coords(double re, double im, const unsigned int R) {
  //printf("(%lf, %lf)", re, im);
  double re_scale = (double)R/3;
  double im_scale = (double)R/2;

  re = re + 2.0;
  im = im + 1.0;

  struct coords_t coords;
  coords.x = (int)(re*re_scale);
  coords.y = (int)(im*im_scale);

  return coords;
}

struct point_t split_line(char *line) {
  char *val[3];
  for(int i = 0; i<3; i++) {
    val[i] = strsep(&line, ",");
  }
  char *ptr1, *ptr2, *ptr3;

  struct point_t point;
  point.re = strtod(val[0], &ptr1);
  point.im = strtod(val[1], &ptr2);
  point.i = strtol(val[2], &ptr3, 0);

  //printf("<%.16lf, %.16lf>\n", point.re, point.im);
  return point;
}

int main(int argc, char **argv) {

  if(argc < 3) {
      errno = EINVAL;
      perror("Not enough arguments");
      exit(1);
  }
  const unsigned int R = atoi(argv[2]);
  long int **data_array;
  data_array = malloc(R*sizeof(long int*));
  for(unsigned int i = 0; i<R; i++) {
    data_array[i] = calloc(R, sizeof(long int));
  }

  if(mkfifo(argv[1], 0666) == -1) {
    perror("Cannot create fifo pipe");
  }

  printf("Fifo created.\n");

  int fd;
  if((fd = open(argv[1], O_RDONLY)) == -1) {
    perror("Error opening pipe");
    exit(1);
  }

  printf("Fifo opened.\n");
  char buffer[MAX_BUF] = {0x0};
  while(read(fd, buffer, MAX_BUF) > 0) {
    //printf("[[%s]]\n", buffer);
    struct point_t point = split_line(buffer);
    struct coords_t coords = get_coords(point.re, point.im, R);
    //printf("%d, %d, %ld\n", coords.x, coords.y, point.i);
    data_array[coords.x][coords.y] = point.i;
  }

  close(fd);
  unlink(argv[1]);

  FILE *data_handler = NULL;
  if((data_handler = fopen("data", "w")) == NULL) {
    perror("Error creating/opening data file");
    exit(1);
  }

  for(unsigned int i = 0; i < R; i++) {
    for(unsigned int j = 0; j < R; j++) {
      char buffer[MAX_BUF] = {0x0};
      sprintf(buffer, "%u %u %ld\n", i, j, data_array[i][j]);
      fwrite(buffer, strlen(buffer), 1, data_handler);
    }
  }

  fclose(data_handler);

  FILE *gnuplot_handler = NULL;
  if((gnuplot_handler = popen("gnuplot", "w")) == NULL) {
    perror("Error creating/opening gnuplot pipe");
    exit(1);
  }
  fputs("set view map\n", gnuplot_handler);
  fputs("set xrange [0:600]\n", gnuplot_handler);
  fputs("set yrange [0:600]\n", gnuplot_handler);
  fputs("plot 'data' with image\n", gnuplot_handler);
  printf("Hej");
  fflush(gnuplot_handler);
  getc(stdin);

  pclose(gnuplot_handler);
  return 0;
}
