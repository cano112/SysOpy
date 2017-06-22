#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/un.h>
#include <signal.h>
#include <pthread.h>
#include <sys/time.h>
#include <string.h>
#include <limits.h>
#include <time.h>

#define NAME_LENGTH 16
#define MSG_LENGTH 200

typedef enum connetion_type {
  IPV4,
  LOCAL,
  UNDEFINED
} connection_type;

typedef enum order_type {
  ADD,
  SUBTRACT,
  DIVIDE,
  MULTIPLY
} order_type;

typedef struct message {
  int msg_id;
  char sender[NAME_LENGTH];
  order_type type;
  int arg_1;
  int arg_2;
  int result;
  char message[MSG_LENGTH];
} message;

int sock;
struct sockaddr_in net_addr;
struct sockaddr_un loc_addr;
struct sockaddr *server;
char *client_name;
char client_path[5];

int random_int(int from, int to) {
  return rand()%(to-from+1)+from;
}

int send_message(char *name, char *content, int msg_id) {
    message msg;
    memset(&msg, 0, sizeof(message));
    if (strncpy(msg.sender, name, NAME_LENGTH-1) == NULL) {
      return -1;
    }
    if (strncpy(msg.message, content, MSG_LENGTH-1) == NULL) {
      return -1;
    }
    msg.msg_id = msg_id;

    if (sendto(sock, &msg, sizeof(msg), 0, server, sizeof(*server)) == -1) {
      return -1;
    }
    return 0;
}

void sigint_handler(int signum) {
  if(send_message(client_name, "DELETE ME", -3) == -1) {
    perror("Cannot send message to server\n");
    exit(-1);
  }
  exit(0);
}

void exit_handler() {
  if (close(sock) == -1) {
    perror("Error ");
  }

  if (unlink(client_path) == -1) {
    perror("Cannot unlink socket file descriptor");
  }
}

void create_net_socket(char *name, char *address, int port) {



    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
      perror("Error getting socket descriptor");
      exit(-1);
    }

    memset(&net_addr, 0, sizeof(net_addr));
    net_addr.sin_family = AF_INET;
    net_addr.sin_port = htons(port);
    net_addr.sin_addr.s_addr = inet_addr(address);

    server = (struct sockaddr*)&net_addr;
}

void create_local_socket(char *name, char *socket_path) {

    struct sockaddr_un client_addr;

    memset(&loc_addr, 0, sizeof(loc_addr));
    loc_addr.sun_family = AF_UNIX;
    strncpy(loc_addr.sun_path, socket_path, sizeof(loc_addr.sun_path) - 1);
    loc_addr.sun_path[sizeof(loc_addr.sun_path) - 1] = '\0';

    client_path[0] = '.';
    client_path[1] = '/';
    client_path[2] = (char)random_int('0', '9');
    client_path[3] = (char)random_int('0', '9');
    client_path[4] = '\0';

    memset(&client_addr, 0, sizeof(client_addr));
    client_addr.sun_family = AF_UNIX;
    strncpy(client_addr.sun_path, client_path, sizeof(client_addr.sun_path) - 1);
    client_addr.sun_path[sizeof(client_addr.sun_path) - 1] = '\0';

    if ((sock = socket(AF_UNIX, SOCK_DGRAM, 0)) == -1) {
      perror("Error getting socket descriptor");
      exit(-1);
    }

    if(bind(sock, (struct sockaddr *)&client_addr, sizeof(client_addr)) < 0) {
      perror("Binding socket error");
      exit(-1);
    }

    if(connect(sock, (struct sockaddr *)&loc_addr, sizeof(loc_addr)) < 0) {
      perror("Connect socket error");
      exit(-1);
    }

    server = (struct sockaddr*)&loc_addr;
}


int handle_order(char *name, order_type type, int arg_1, int arg_2, int msg_id) {
  message msg;
  memset(&msg, 0, sizeof(message));

  msg.msg_id = msg_id;
  msg.type = type;
  if (strncpy(msg.sender, name, NAME_LENGTH-1) == NULL) {
    return -1;
  }

  switch(type) {
    case ADD:
      msg.result = arg_1+arg_2;
      break;
    case SUBTRACT:
      msg.result = arg_1-arg_2;
      break;
    case MULTIPLY:
      msg.result = arg_1*arg_2;
      break;
    case DIVIDE:
      if(arg_2 != 0) msg.result = arg_1/arg_2;
      else {
        msg.result = INT_MAX;
        msg.msg_id = -2;
      }
      break;
  }

  if (sendto(sock, &msg, sizeof(msg), 0, server, sizeof(*server)) == -1) {
    return -1;
  }

  return 0;
}

int main(int argc, char **argv) {
  srand(time(NULL));
  signal(SIGINT, sigint_handler);
  if(atexit(exit_handler) == -1) {
    perror("Cannot define exit handler");
    exit(-1);
  }
  if(argc < 4) {
    errno = EINVAL;
    perror("./client <name> loc|net <address>");
    exit(-1);
  }
  client_name = argv[1];
  connection_type connection_type = (strcmp("net", argv[2]) == 0) ? IPV4 : (strcmp("loc", argv[2]) == 0) ? LOCAL : UNDEFINED;
  if(connection_type == LOCAL) {
    create_local_socket(client_name, argv[3]);
  }
  else {
    if(connection_type == IPV4) {
      char *addr_port = argv[3];
      char *addr = strsep(&addr_port, ":");
      char *port = strsep(&addr_port, ":");
      create_net_socket(client_name, addr, atoi(port));
    }
    else {
      errno = EINVAL;
      perror("There's no such connection type");
      exit(-1);
    }
  }

  if(send_message(client_name, "Hello, server!", 0) == -1) {
    perror("Cannot send message");
    exit(-1);
  }

  message msg;
  while(1) {
    socklen_t size = sizeof(*server);
    if(recvfrom(sock, &msg, sizeof(message), 0, server, &size) > 0) {

      if(msg.msg_id == -1) {
        printf("%s: %s\n",msg.sender, msg.message);
        exit(-1);
      }
      else {
        if(msg.msg_id == 0) {
          printf("%s: %s\n",msg.sender, msg.message);
        }
        else {
          if(msg.msg_id > 0) {
            printf("Order received. Type: %d, arg_1: %d, arg_2: %d\n", msg.type, msg.arg_1, msg.arg_2);
            if(handle_order(client_name, msg.type, msg.arg_1, msg.arg_2, msg.msg_id) == -1) {
              perror("Cannot send order result");
              exit(-1);
            }

          }
        }
      }
    }
  }

  return 0;
}
