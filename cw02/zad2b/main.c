#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <libgen.h>
#include <limits.h>
#include <unistd.h>
#include <time.h>
#include <ftw.h>
#include <string.h>
#include <stdint.h>

int FILE_SIZE;

static int write_file_info(const char *fpath, const struct stat *buffer, int tflag, struct NTW *ftwbuf) {

  if(tflag == FTW_F && buffer->st_size <= FILE_SIZE) {
    char perm[11] = "----------";
    if(buffer->st_mode & S_IRUSR) perm[1] = 'r';
    if(buffer->st_mode & S_IWUSR) perm[2] = 'w';
    if(buffer->st_mode & S_IXUSR) perm[3] = 'x';
    if(buffer->st_mode & S_IRGRP) perm[4] = 'r';
    if(buffer->st_mode & S_IWGRP) perm[5] = 'w';
    if(buffer->st_mode & S_IXGRP) perm[6] = 'x';
    if(buffer->st_mode & S_IROTH) perm[7] = 'r';
    if(buffer->st_mode & S_IWOTH) perm[8] = 'w';
    if(buffer->st_mode & S_IXOTH) perm[9] = 'x';

    if(buffer->st_mode & S_ISUID) perm[3] = 's';
    if(buffer->st_mode & S_ISGID) perm[6] = 's';
    if(buffer->st_mode & S_ISVTX) perm[9] = 't';

    printf("%s\n%ldB\n%s\n",fpath, buffer->st_size, perm);

    char buff[100];
    strftime(buff, sizeof buff, "%d-%m-%Y %X", localtime(&(buffer->st_mtim.tv_sec)));
    printf("%s\n\n", buff);
  }
  return 0;
}

int main(int argc, char **argv) {

  if(argc < 3) {
    errno = EINVAL;
    perror("Not enough arguments");
    exit(1);
  }
  int flags = 0;
  flags |=FTW_PHYS;
  FILE_SIZE = atoi(argv[2]);

  if (nftw(argv[1], write_file_info, 20, flags) == -1) {
        perror("nftw");
        exit(1);
  }

  return 0;
}
