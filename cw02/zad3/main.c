#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>


void fflushstdin( void )
{
    int c;
    while( (c = fgetc( stdin )) != EOF && c != '\n' );
}

void write_out_menu() {
  printf("\nWhat would you like to do?\n[1] Set read lock\n[2] Set write lock\n[3] List all locks\n[4] Unset lock\n[5] read character\n[6] change character\n");
}

int set_lock(int fd, char mode, char wait, int byte) {

  struct flock fl;

  fl.l_type = (mode == 'r') ? F_RDLCK : (mode == 'w') ? F_WRLCK : F_UNLCK;
  fl.l_whence =  SEEK_SET;
  fl.l_start = byte;
  fl.l_len = 1;
  fl.l_pid = getpid();

  if(wait == 'y') {
    if (fcntl(fd, F_SETLKW, &fl) == -1) {
        return 1;
    }
  }
  else {
    if (fcntl(fd, F_SETLK, &fl) == -1) {
        return 1;
    }
  }

  return 0;
}

int write_out_locks(int fd) {

  struct flock fl;

  int file_length = lseek(fd, 0, SEEK_END);
  lseek(fd,0,SEEK_SET);


  int i = 0;
  do {
    fl.l_start = i;
    fl.l_type = F_WRLCK;
    fl.l_whence =  SEEK_SET;
    fl.l_len = 1;

    if (fcntl(fd, F_GETLK, &fl) == -1) {
        return 1;
    }
    if(fl.l_type == F_WRLCK) {
      printf("LOCK ON BYTE #%d: WRITE %d\n",i, fl.l_pid);
    }
    else {
      if(fl.l_type == F_RDLCK) {
        printf("LOCK ON BYTE #%d: READ %d\n",i, fl.l_pid);
      }
    }

    i++;

  } while (i < file_length);

  return 0;
}

int read_byte(int fd, int byte, char *buf) {

  if(lseek(fd, byte, SEEK_SET) == -1) {
    return 1;
  }

  if(read(fd, buf, 1) == -1) {
    return 1;
  }

  return 0;
}

int write_byte(int fd, int byte, char new_byte) {

  if(lseek(fd, byte, SEEK_SET) == -1) {
    return 1;
  }

  if(write(fd, &new_byte, 1) == -1) {
    return 1;
  }

  return 0;
}

int main(int argc, char **argv) {

    if(argc<2) {
      errno = EINVAL;
      perror("Not enough arguments");
      exit(1);
    }

    int fd;
    if ((fd = open(argv[1], O_RDWR)) == -1) {
          perror("Cannot open file");
          return 1;
    }

    while(1) {
      write_out_menu();
      int option;
      int byte;
      char wait;
      scanf("%d", &option);

      switch(option) {
        case 1:
          printf("Which byte? ");
          scanf("%d", &byte);
          fflushstdin();
          printf("With wait? [y/n]");
          scanf("%c", &wait);
          fflushstdin();
          if (set_lock(fd, 'r', wait, byte) != 0) {
            perror("Cannot set lock");
            continue;
          }
          printf("#%d byte locked\n", byte);
          break;

        case 2:
          printf("Which byte? ");
          scanf("%d", &byte);
          fflushstdin();
          printf("With wait? [y/n]");
          scanf("%c", &wait);
          fflushstdin();
          if (set_lock(fd, 'w', wait, byte) != 0) {
            perror("Cannot set lock");
            continue;
          }
          printf("#%d byte locked\n", byte);
          break;

        case 3:
          if(write_out_locks(fd) != 0) {
            perror("Error writing out locks");
          }
          break;

        case 4:
          printf("Which byte? ");
          scanf("%d", &byte);
          fflushstdin();
          if(set_lock(fd, 'u', 'n', byte) != 0) {
            perror("Cannot unset lock");
            continue;
          }
          printf("Lock succesfully unset\n");
          break;

        case 5:
          printf("Which byte? ");
          scanf("%d", &byte);
          fflushstdin();
          char buf;
          if (read_byte(fd, byte, &buf) != 0) {
            perror("Error reading from file");
            continue;
          }
          printf("#%d byte: %c", byte, buf);
          break;

        case 6:
          printf("Which byte? ");
          scanf("%d", &byte);
          fflushstdin();
          printf("Replace with?");
          scanf("%c", &wait);
          fflushstdin();
          if(write_byte(fd, byte, wait) != 0) {
            perror("Error writing into file");
            continue;
          }
          printf("Byte succesfully replaced.\n");
          break;

      }
    }
    close (fd);
    return 0;
}
