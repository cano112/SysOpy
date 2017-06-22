#include <stdio.h>
#include <stdlib.h>
#include <mqueue.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include "constants.h"

int server_d;
int client_d;
int client_id;
char *client_name;

char *rand_name(size_t size) {
  const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJK";
  char *str = malloc(size+1);
  if (size) {
        --size;
        str[0] = '/';
        for (size_t n = 1; n < size; n++) {
            int key = rand() % (int) (sizeof charset - 1);
            str[n] = charset[key];
        }
        str[size] = '\0';
    }

    return str;
}

void exit_fun() {

  char buf[MAX_MSG_LEN+2];
  buf[0] = '0' + CLOSE_QUEUE_MSG;
  buf[1] = '0' + client_id;

  if((mq_send(server_d, buf, MAX_MSG_LEN+2,  1)) == -1) {
    perror("Cannot send msg to server");
    exit(1);
  }
  mq_close(client_d);
  mq_unlink(client_name);
}
void print_menu() {
  printf("[0] ZAKONCZ SERWER\n[1] ECHO\n[2] WERSALIKI\n[3] CZAS\n[Q] ZAKONCZ PROGRAM\n");
}

int send_task(int q_d, int client_id, int task, char *text) {
  char buf[MAX_MSG_LEN+2];
  buf[0] = '0' + task;
  buf[1] = '0' + client_id;
  sprintf(buf+2, "%s", text);

  if((mq_send(q_d, buf, MAX_MSG_LEN+2,  1)) == -1) {
    return -1;
  }
  return 0;
}

void task(int mode) {
  char msg[MAX_MSG_LEN];
  char buf[MAX_MSG_LEN+2];
  if(mode != TASK_TIME_MSG) {
    fgets(msg, MAX_MSG_LEN, stdin);
    int len = strlen(msg);
    if ((len>0) && (msg[len - 1] == '\n'))
        msg[len - 1] = '\0';
  }

  if(send_task(server_d, client_id, mode, msg) == -1) {
    perror("Cannot send task");
  }
  if(mq_receive(client_d, buf, MAX_MSG_LEN+2, NULL) == -1) {
    perror("Cannot receive response from server");
  }
  printf("%s\n", buf+2);
}

int main(void) {
  srand(time(NULL));
  atexit(exit_fun);

  client_name = rand_name(7);

  server_d = mq_open("/server_queue", O_RDWR);
  if(server_d == -1) {
    perror("Cannot open msg queue");
    exit(1);
  }

  struct mq_attr attr;
  attr.mq_flags = 0;
  attr.mq_maxmsg = QUEUE_SIZE;
  attr.mq_msgsize = MAX_MSG_LEN+2;
  attr.mq_curmsgs = 0;
  client_d = mq_open(client_name, O_RDONLY | O_CREAT, 0666, &attr);
  if(client_d == -1) {
    perror("Cannot create msg queue");
    exit(1);
  }

  char buf[MAX_MSG_LEN+2];
  buf[0] = '0' + INIT_QUEUE_MSG;
  sprintf(buf+1, "%s", client_name);

  if((mq_send(server_d, buf, MAX_MSG_LEN+2,  1)) == -1) {
    perror("Cannot send msg to server");
    exit(1);
  }

  if(mq_receive(client_d, buf, MAX_MSG_LEN+2, NULL) == -1 || buf[0] != INIT_QUEUE_RET_MSG + '0') {
    perror("Cannot receive msg");
    exit(1);
  }
  client_id = atoi(buf+1);
  printf("Client id: %d\n", client_id);

  while(1) {
    print_menu();
    int m = getchar();
    int c;
    while ((c = getchar()) != '\n' && c != EOF) { }

    switch(m) {
      case '0':
        buf[0] = '0' + TASK_END_MSG;
        if((mq_send(server_d, buf, MAX_MSG_LEN+2, 1)) == -1) {
          perror("Cannot send task");
        }
        exit(0);
        break;
      case '1':
        task(TASK_ECHO_MSG);
        break;
      case '2':
        task(TASK_CAPS_MSG);
        break;
      case '3':
        task(TASK_TIME_MSG);
        break;
      case 'q':
      case 'Q':
        exit(0);
    }
  }

  return 0;
}
