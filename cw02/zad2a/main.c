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


void write_file_info(char *real_path, int size_condition) {
  struct stat *buffer = malloc(sizeof(struct stat));
  if(lstat(real_path, buffer) == -1) {
    perror("Cannot get info about file");
    exit(1);
  }
  else {
    if(buffer->st_size <= size_condition) {
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

      printf("%s\n%ldB\n%s\n",real_path, buffer->st_size, perm);

      char buff[100];
      strftime(buff, sizeof buff, "%d-%m-%Y %X", localtime(&(buffer->st_mtim.tv_sec)));
      printf("%s\n\n", buff);
    }
  }
  free(buffer);
}
void dir_traversal(DIR *dir_handler, int file_size, char *path) {

  struct dirent *dirent = NULL;
  errno = 0;

  while(errno == 0 && (dirent = readdir(dir_handler)) != NULL) {

    char new_path[PATH_MAX+1];
    strcpy(new_path, path);
    strcat(new_path, "/");
    strcat(new_path, dirent->d_name);

    if(dirent->d_type == DT_DIR && strcmp(dirent->d_name,"..") != 0 && strcmp(dirent->d_name,".") != 0) {
      DIR *sub_dir_handler;
      sub_dir_handler = opendir(new_path);
      if(sub_dir_handler == NULL) {
        perror("Cannot open given directory");
      }
      dir_traversal(sub_dir_handler, file_size, new_path);

    }
    else {
      if(dirent->d_type == DT_REG) {
        char real_path[PATH_MAX + 1];
        char *res = realpath(new_path, real_path);
        if(res != NULL) {
          write_file_info(real_path, file_size);
        }
        else {
          perror("Cannot resolve to real path");
          exit(1);
        }
      }
    }
  }

  if(errno != 0) {
    perror("Cannot read a directory");
    exit(1);
  }

  free(dirent);

  if(closedir(dir_handler) == -1) {
    perror("Cannot close a directory");
    exit(1);
  }
}

int main(int argc, char **argv) {

  if(argc < 3) {
    errno = EINVAL;
    perror("Not enough arguments");
    exit(1);
  }
  char path[PATH_MAX+1];
  strcpy(path, argv[1]);
  DIR *dir_handler;
  dir_handler = opendir(path);
  if(dir_handler == NULL) {
    perror("Cannot open given directory");
  }

  dir_traversal(dir_handler, atoi(argv[2]), path);


  return 0;
}
