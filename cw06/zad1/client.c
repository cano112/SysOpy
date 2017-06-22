#include <stdio.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "constants.h"

int server_q_id;
int client_q_id;
int client_id;

void exit_fun() {
  msgctl(client_q_id, IPC_RMID, NULL);
}
void print_menu() {
  printf("[0] ZAKONCZ SERWER\n[1] ECHO\n[2] WERSALIKI\n[3] CZAS\n[Q] ZAKONCZ PROGRAM\n");
}

int send_task(int q_id, int client_id, long task, char *text) {
  struct msgbuffer buf;
  buf.type = task;
  buf.id = client_id;
  sprintf(buf.text, "%s", text);

  if((msgsnd(q_id, &buf, sizeof(buf) - sizeof(long),  0)) == -1) {
    return -1;
  }
  return 0;
}

void task(int mode) {
  char msg[MAX_MSG_LEN];
  struct msgbuffer buf;
  if(mode != TASK_TIME_MSG) {
    fgets(msg, MAX_MSG_LEN, stdin);
    int len = strlen(msg);
    if ((len>0) && (msg[len - 1] == '\n'))
        msg[len - 1] = '\0';
  }

  if(send_task(server_q_id, client_id, mode, msg) == -1) {
    perror("Cannot send task");
  }
  if(msgrcv(client_q_id, &buf, sizeof(buf) - sizeof(long), mode, 0) == -1) {
    perror("Cannot receive response from server");
  }
  printf("%s\n", buf.text);
}

int main(void) {
  atexit(exit_fun);
  key_t server_key = ftok(HOME, SERVER_P);

  if((server_q_id = msgget(server_key, 0)) == -1) {
    perror("Cannot open msg queue");
    exit(1);
  }

  key_t client_key = ftok(HOME, CLIENT_P);

  if((client_q_id = msgget(client_key, IPC_CREAT | 0660)) == -1) {
    perror("Cannot create msg queue");
    exit(1);
  }

  struct msgbuffer buf;
  buf.type = INIT_QUEUE_MSG;
  sprintf(buf.text, "%d", client_key);

  if((msgsnd(server_q_id, &buf, sizeof(buf) - sizeof(long),  0)) == -1) {
    perror("Cannot send msg to server");
    exit(1);
  }

  if(msgrcv(client_q_id, &buf, sizeof(buf) - sizeof(long), INIT_QUEUE_RET_MSG, 0) == -1) {
    perror("Cannot receive msg");
    exit(1);
  }
  client_id = atoi(buf.text);
  printf("Client id: %d\n", client_id);

  while(1) {
    print_menu();
    int m = getchar();
    int c;
    while ((c = getchar()) != '\n' && c != EOF) { }

    switch(m) {
      case '0':
        buf.type = TASK_END_MSG;
        if((msgsnd(server_q_id, &buf, sizeof(buf) - sizeof(long),  0)) == -1) {
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
