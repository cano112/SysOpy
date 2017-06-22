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

    if (write(sock, &msg, sizeof(msg)) == -1) {
      return -1;
    }
    return 0;
}

void sigint_handler(int signum) {
  if(send_message("", "DELETE ME", -3) == -1) {
    perror("Cannot send message to server\n");
    exit(-1);
  }
  exit(0);
}

void exit_handler() {

  if (shutdown(sock, SHUT_RDWR) == -1) {
    perror("Error shutting down socket");
  }
  if (close(sock) == -1) {
    perror("Error ");
  }
}

void create_net_socket(char *name, char *address, int port) {

    struct sockaddr_in addr;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
      perror("Error getting socket descriptor");
      exit(-1);
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if (inet_pton(AF_INET, address, &addr.sin_addr) != 1) {
          perror("Incorrent IP address");
          exit(-1);
    }
    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
      perror("Error connecting to server");
      exit(-1);
    }
}

void create_local_socket(char *name, char *socket_path) {
    struct sockaddr_un addr;
    if ((sock = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
      perror("Error getting socket descriptor");
      exit(-1);
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);
    addr.sun_path[sizeof(addr.sun_path) - 1] = '\0';

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
      perror("Error connecting to server");
      exit(-1);
    }
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

  if (write(sock, &msg, sizeof(msg)) == -1) {
    return -1;
  }
  return 0;
}
int main(int argc, char **argv) {

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

  connection_type connection_type = (strcmp("net", argv[2]) == 0) ? IPV4 : (strcmp("loc", argv[2]) == 0) ? LOCAL : UNDEFINED;
  if(connection_type == LOCAL) {
    create_local_socket(argv[1], argv[3]);
  }
  else {
    if(connection_type == IPV4) {
      char *addr_port = argv[3];
      char *addr = strsep(&addr_port, ":");
      char *port = strsep(&addr_port, ":");
      create_net_socket(argv[1], addr, atoi(port));
    }
    else {
      errno = EINVAL;
      perror("There's no such connection type");
      exit(-1);
    }
  }

  if(send_message(argv[1], "Hello, server!", 0) == -1) {
    perror("Cannot send message");
    exit(-1);
  }

  message msg;
  while(1) {
    if(read(sock, &msg, sizeof(message)) > 0) {

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
            if(handle_order(argv[1], msg.type, msg.arg_1, msg.arg_2, msg.msg_id) == -1) {
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
