#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

void generate(char *path, int rec_c, int rec_l) {
  int file_handler;
  if((file_handler = open(path, O_WRONLY|O_CREAT|O_TRUNC, 00644)) == -1) {
    perror("Failed to open file");
    exit(1);
  }

  int rand_handler;
  if((rand_handler = open("/dev/random",O_RDONLY)) == -1) {
      perror("Failder to open random generator");
      exit(1);
  }

  int ret;
  unsigned char *buffer = malloc(rec_l*sizeof(char));

  for(int i = 0; i<rec_c; i++) {
    if((ret = read(rand_handler, buffer, rec_l)) == -1) {
      perror("Failed to read from random generator");
      exit(1);
    }

    if((ret = write(file_handler, buffer, rec_l)) == -1) {
      perror("Failed to write to file");
      exit(1);
    }
  }

  free(buffer);

  if(close(rand_handler) == -1) {
    perror("Failed to close random generator");
  }
  if(close(file_handler) == -1) {
    perror("Failed to close file");
  }
}

int sys_compare_records(int file_handler, int rec_l, int off1, int off2) {
  unsigned char *buffer1 = malloc(rec_l*sizeof(char));
  unsigned char *buffer2 = malloc(rec_l*sizeof(char));
  int ret;

  /* Fill buffers with records */
  if(lseek(file_handler,off1, SEEK_SET) == -1) {
    perror("Failed to change cursor position");
    exit(1);
  }

  if((ret = read(file_handler, buffer1, rec_l)) == -1) {
    perror("Failed to read from file");
    exit(1);
  }

  if(lseek(file_handler,off2, SEEK_SET) == -1) {
    perror("Failed to change cursor position");
    exit(1);
  }

  if((ret = read(file_handler, buffer2, rec_l)) == -1) {
    perror("Failed to read from file");
    exit(1);
  }

  int result;
  /* compare first chars */
  if (buffer1[0] > buffer2[0]) {
    result = 1;
  }
  else {
    if(buffer1[0] == buffer2[0]) {
      result = 0;
    }
    else {
      result = -1;
    }
  }

  free(buffer1);
  free(buffer2);

  return result;
}

void sys_swap_records(int file_handler, int rec_l, int off1, int off2) {

  unsigned char *buffer1 = malloc(rec_l*sizeof(char));
  unsigned char *buffer2 = malloc(rec_l*sizeof(char));
  int ret;

  /* Fill buffers with records */
  if(lseek(file_handler,off1, SEEK_SET) == -1) {
    perror("Failed to change cursor position");
    exit(1);
  }

  if((ret = read(file_handler, buffer1, rec_l)) == -1) {
    perror("Failed to read from file");
    exit(1);
  }

  if(lseek(file_handler,off2, SEEK_SET) == -1) {
    perror("Failed to change cursor position");
    exit(1);
  }

  if((ret = read(file_handler, buffer2, rec_l)) == -1) {
    perror("Failed to read from file");
    exit(1);
  }

  /* Fill 2nd record with 1st record value */
  if(lseek(file_handler,off2, SEEK_SET) == -1) {
    perror("Failed to change cursor position");
    exit(1);
  }

  if((ret = write(file_handler, buffer1, rec_l)) == -1) {
    perror("Failed to write to file");
    exit(1);
  }

  /* Fill 1st record with 2nd record value */
  if(lseek(file_handler,off1, SEEK_SET) == -1) {
    perror("Failed to change cursor position");
    exit(1);
  }

  if((ret = write(file_handler, buffer2, rec_l)) == -1) {
    perror("Failed to write to file");
    exit(1);
  }

  free(buffer1);
  free(buffer2);
}

void sys_shuffle(char *path, int rec_c, int rec_l) {
  int file_handler;
  if((file_handler = open(path, O_RDWR)) == -1) {
    perror("Failed to open file");
    exit(1);
  }

  int random_index;
  for(int i = rec_c-1; i >= 1; i--) {
    random_index = (rand()%rec_c);
    sys_swap_records(file_handler, rec_l, random_index*rec_l, i*rec_l);
  }

  if(close(file_handler) == -1) {
    perror("Failed to close file");
  }
}

void sys_sort(char *path, int rec_c, int rec_l) {
  int file_handler;
  if((file_handler = open(path, O_RDWR)) == -1) {
    perror("Failed to open file");
    exit(1);
  }
  do {
    for(int i = 0; i < rec_c-1; i++) {
      if(sys_compare_records(file_handler, rec_l, i*rec_l, (i+1)*rec_l) == 1) {
        sys_swap_records(file_handler, rec_l, i*rec_l, (i+1)*rec_l);
      }
    }
    rec_c--;
  } while (rec_c > 1);

  if(close(file_handler) == -1) {
    perror("Failed to close file");
  }
}

void lib_swap_records(FILE *file_handler, int rec_l, int off1, int off2) {

  unsigned char *buffer1 = malloc(rec_l*sizeof(char));
  unsigned char *buffer2 = malloc(rec_l*sizeof(char));
  int ret;

  /* Fill buffers with records */
  fseek(file_handler,off1, SEEK_SET);

  ret = fread(buffer1, 1, rec_l, file_handler);
  if(ret != rec_l) {
    fputs("Reading from file error", stderr);
    exit(1);
  }

  fseek(file_handler,off2, SEEK_SET);

  ret = fread(buffer2, 1, rec_l, file_handler);
  if(ret != rec_l) {
    fputs("Reading from file error", stderr);
    exit(1);
  }

  /* Fill 2nd record with 1st record value */
  fseek(file_handler,off2, SEEK_SET);

  ret = fwrite(buffer1, 1, rec_l, file_handler);
  if(ret != rec_l) {
    fputs("Writing into file error", stderr);
    exit(1);
  }

  /* Fill 1st record with 2nd record value */
  fseek(file_handler,off1, SEEK_SET);

  ret = fwrite(buffer2, 1, rec_l, file_handler);
  if(ret != rec_l) {
    fputs("Writing into file error", stderr);
    exit(1);
  }

  free(buffer1);
  free(buffer2);
}

int lib_compare_records(FILE *file_handler, int rec_l, int off1, int off2) {
  unsigned char *buffer1 = malloc(rec_l*sizeof(char));
  unsigned char *buffer2 = malloc(rec_l*sizeof(char));
  int ret;

  /* Fill buffers with records */
  fseek(file_handler,off1, SEEK_SET);

  ret = fread(buffer1, 1, rec_l, file_handler);
  if(ret != rec_l) {
    fputs("Reading from file error", stderr);
    exit(1);
  }

  fseek(file_handler,off2, SEEK_SET);

  ret = fread(buffer2, 1, rec_l, file_handler);
  if(ret != rec_l) {
    fputs("Reading from file error", stderr);
    exit(1);
  }

  int result;
  /* compare first chars */
  if (buffer1[0] > buffer2[0]) {
    result = 1;
  }
  else {
    if(buffer1[0] == buffer2[0]) {
      result = 0;
    }
    else {
      result = -1;
    }
  }

  free(buffer1);
  free(buffer2);

  return result;
}

void lib_shuffle(char *path, int rec_c, int rec_l) {
  FILE *file_handler;
  file_handler = fopen(path,"r+b");
  if(ferror(file_handler)) {
    fputs("Opening file error", stderr);
    exit(1);
  }

  int random_index;
  for(int i = rec_c-1; i >= 1; i--) {
    random_index = (rand()%rec_c);
    lib_swap_records(file_handler, rec_l, random_index*rec_l, i*rec_l);
  }

  fclose(file_handler);
}

void lib_sort(char *path, int rec_c, int rec_l) {
  FILE *file_handler;
  file_handler = fopen(path,"r+b");
  if(ferror(file_handler)) {
    fputs("Opening file error", stderr);
    exit(1);
  }

  do {
    for(int i = 0; i < rec_c-1; i++) {
      if(lib_compare_records(file_handler, rec_l, i*rec_l, (i+1)*rec_l) == 1) {
        lib_swap_records(file_handler, rec_l, i*rec_l, (i+1)*rec_l);
      }
    }
    rec_c--;
  } while (rec_c > 1);

  fclose(file_handler);
}

int main(int argc, char **argv) {
  srand(time(NULL));

  if(argc<5) {
    errno = EINVAL;
    perror("Not enough arguments");
    exit(1);
  }
  else {
    if(strcmp(argv[1],"sys") == 0 && argc >= 6) {
      if(strcmp(argv[2],"shuffle") == 0) {
        sys_shuffle(argv[3], atoi(argv[4]), atoi(argv[5]));
      }
      else {
        if(strcmp(argv[2],"sort") == 0) {
          sys_sort(argv[3], atoi(argv[4]), atoi(argv[5]));
        }
      }
    }
    else {
      if(strcmp(argv[1],"lib") == 0 && argc >= 6) {
        if(strcmp(argv[2],"shuffle") == 0) {
          lib_shuffle(argv[3], atoi(argv[4]), atoi(argv[5]));
        }
        else {
          if(strcmp(argv[2],"sort") == 0) {
            lib_sort(argv[3], atoi(argv[4]), atoi(argv[5]));
          }
        }
      }
      else {
        if(strcmp(argv[1],"generate") == 0) {
          generate(argv[2], atoi(argv[3]), atoi(argv[4]));
        }
      }
    }
  }


  return 0;
}
