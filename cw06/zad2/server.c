#include <stdio.h>
#include <mqueue.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include "constants.h"

mqd_t server_d;
mqd_t clients[CLIENTS_NO];
int cur_client_no = 0;

void exit_fun() {
  for(int i = 0; i < CLIENTS_NO; i++) {
    mq_close(clients[i]);
  }
  mq_close(server_d);
  mq_unlink("/server_queue");
}

void init_queue(char *buf) {
  printf("Client queue init: %s\n", buf+1);

  clients[cur_client_no++] = mq_open(buf+1, O_WRONLY);
  if(clients[cur_client_no-1] == -1) {
    perror("Cannot open client queue");
    exit(1);
  }

  buf[0] = '0' + INIT_QUEUE_RET_MSG;
  sprintf(buf+1, "%d", cur_client_no-1);

  if((mq_send(clients[cur_client_no-1], buf, MAX_MSG_LEN+2,  1)) == -1) {
    perror("Cannot send msg to client");
    exit(1);
  }

}

void task_echo(char *buf) {
  if((mq_send(clients[buf[1]-'0'], buf, MAX_MSG_LEN+2,  1)) == -1) {
    perror("Cannot send msg to client");
    exit(1);
  }
}

void task_caps(char *buf) {

  for(unsigned int i = 0; i<strlen(buf+2); i++) {
    buf[2+i] = toupper(buf[2+i]);
  }
  if((mq_send(clients[buf[1]-'0'], buf, MAX_MSG_LEN+2,  1)) == -1) {
    perror("Cannot send msg to client");
    exit(1);
  }
}

void task_time(char *buf) {

  time_t t = time(NULL);
  struct tm tm = *localtime(&t);
  sprintf(buf+2, "%d-%d-%d %d:%d:%d",
    tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
    tm.tm_hour, tm.tm_min, tm.tm_sec);
  if((mq_send(clients[buf[1]-'0'], buf, MAX_MSG_LEN+2,  1)) == -1) {
    perror("Cannot send msg to client");
    exit(1);
  }
}

void close_queue(char *buf) {
  mq_close(clients[buf[1]-'0']);
}

int main(void) {

  atexit(exit_fun);

  struct mq_attr attr;
  attr.mq_flags = 0;
  attr.mq_maxmsg = QUEUE_SIZE;
  attr.mq_msgsize = MAX_MSG_LEN+2;
  attr.mq_curmsgs = 0;
  server_d = mq_open("/server_queue", O_RDWR | O_CREAT, 0666, &attr);
  if((server_d == -1)) {
    perror("Cannot create msg queue");
    exit(1);
  }

  while(1) {
    char buf[MAX_MSG_LEN+2];
    if(mq_receive(server_d, buf, MAX_MSG_LEN+2, NULL) == -1) {
      perror("Cannot receive msg");
      exit(1);
    }

    switch(buf[0]-'0') {
      case INIT_QUEUE_MSG:
        init_queue(buf);
        break;
      case TASK_ECHO_MSG:
        task_echo(buf);
        break;
      case TASK_CAPS_MSG:
        task_caps(buf);
        break;
      case TASK_TIME_MSG:
        task_time(buf);
        break;
      case TASK_END_MSG:
        exit(0);
        break;
      case CLOSE_QUEUE_MSG:
        close_queue(buf);
        break;
    }
  }

  return 0;
}
