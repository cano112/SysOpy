#ifndef CONSTANTS
#define CONSTANTS

#define HOME getenv("HOME")
#define SERVER_P 'p'
#define CLIENT_P 'c'

#define MAX_MSG_LEN 200
#define CLIENTS_NO 100

enum {
  TASK_END_MSG = 1,
  INIT_QUEUE_MSG,
  INIT_QUEUE_RET_MSG,
  TASK_ECHO_MSG,
  TASK_CAPS_MSG,
  TASK_TIME_MSG
};

struct msgbuffer {
  long type;
  int id;
  char text[MAX_MSG_LEN];
};

#endif
