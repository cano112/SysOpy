#include <stdio.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <time.h>
#include <string.h>
#include "constants.h"

int server_q_id;
int clients[CLIENTS_NO];
int cur_client_no = 0;

void exit_fun() {
  msgctl(server_q_id, IPC_RMID, NULL);
}

void init_queue(struct msgbuffer *buf) {
  printf("Client queue init: %s\n", buf->text);
  key_t client_key = atoi(buf->text);

  if((clients[cur_client_no++] = msgget(client_key, 0)) == -1) {
    perror("Cannot open client queue");
    exit(1);
  }

  buf->type = INIT_QUEUE_RET_MSG;
  sprintf(buf->text, "%d", cur_client_no-1);
  if((msgsnd(clients[cur_client_no-1], buf, sizeof(*buf) - sizeof(long),  0)) == -1) {
    perror("Cannot send msg to client");
    exit(1);
  }
}

void task_echo(struct msgbuffer *buf) {
  if((msgsnd(clients[buf->id], buf, sizeof(*buf) - sizeof(long),  0)) == -1) {
    perror("Cannot send msg to client");
    exit(1);
  }
}

void task_caps(struct msgbuffer *buf) {

  for(unsigned int i = 0; i<strlen(buf->text); i++) {
    buf->text[i] = toupper(buf->text[i]);
  }
  if((msgsnd(clients[buf->id], buf, sizeof(*buf) - sizeof(long),  0)) == -1) {
    perror("Cannot send msg to client");
    exit(1);
  }
}

void task_time(struct msgbuffer *buf) {

  time_t t = time(NULL);
  struct tm tm = *localtime(&t);
  sprintf(buf->text, "%d-%d-%d %d:%d:%d",
    tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
    tm.tm_hour, tm.tm_min, tm.tm_sec);
  if((msgsnd(clients[buf->id], buf, sizeof(*buf) - sizeof(long),  0)) == -1) {
    perror("Cannot send msg to client");
    exit(1);
  }
}

int main(void) {

  atexit(exit_fun);
  key_t key = ftok(HOME, SERVER_P);

  if((server_q_id = msgget(key, IPC_CREAT | 0660)) == -1) {
    perror("Cannot create msg queue");
    exit(1);
  }

  while(1) {
    struct msgbuffer buf;
    if(msgrcv(server_q_id, &buf, sizeof(buf) - sizeof(long), 0, 0) == -1) {
      perror("Cannot receive msg");
      exit(1);
    }

    switch(buf.type) {
      case INIT_QUEUE_MSG:
        init_queue(&buf);
        break;
      case TASK_ECHO_MSG:
        task_echo(&buf);
        break;
      case TASK_CAPS_MSG:
        task_caps(&buf);
        break;
      case TASK_TIME_MSG:
        task_time(&buf);
        break;
      case TASK_END_MSG:
        exit(0);
        break;
    }
  }



  return 0;
}
