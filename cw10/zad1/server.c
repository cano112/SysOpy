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
#include <time.h>

#define BACKLOG 15
#define SOCKETS_NUMBER 20
#define NAME_LENGTH 16
#define MSG_LENGTH 200
#define SERVER_NAME "Krzysztof"

typedef enum order_type {
  ADD,
  SUBTRACT,
  DIVIDE,
  MULTIPLY
} order_type;

typedef struct Client {
  char name[NAME_LENGTH];
  int socket;
} Client;

typedef struct message {
  int msg_id;
  char sender[NAME_LENGTH];
  order_type type;
  int arg_1;
  int arg_2;
  int result;
  char message[MSG_LENGTH];
} message;

char *socket_path;
int socket_net;
int socket_loc;
int max_fd;
Client clients[SOCKETS_NUMBER];
int clients_number = 0;
int order_count = 0;
fd_set set;


void create_net_socket(int port) {

    struct sockaddr_in addr;

    if ((socket_net = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
      perror("Cannot get socket descriptor");
      exit(-1);
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(socket_net, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
      perror("Cannot bind socket");
      exit(-1);
    }
    if (listen(socket_net, BACKLOG) == -1) {
      perror("Listen error");
      exit(-1);
    }
}

void create_local_socket(char* path) {

    struct sockaddr_un addr;

    if ((socket_loc = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
      perror("Cannot get socket descriptor");
      exit(-1);
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);
    addr.sun_path[sizeof(addr.sun_path) - 1] = '\0';

    if (bind(socket_loc, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
      perror("Cannot bind socket");
      exit(-1);
    }

    if (listen(socket_loc, BACKLOG) == -1) {
      perror("Listen error");
      exit(-1);
    }
}

void sigint_handler(int signum) {
  exit(0);
}

void exit_handler() {
  for(int i = 0; i < clients_number; i++) {
    shutdown(clients[i].socket, SHUT_RDWR);
    if (close(clients[i].socket) == -1) {
      perror("Cannot close socket");
    }
  }

  if (close(socket_net) == -1) {
    perror("Cannot close network socket");
  }

  if (close(socket_loc) == -1) {
    perror("Cannot close local socket");
  }

  if (unlink(socket_path) == -1) {
    perror("Cannot unlink socket file descriptor");
  }
}

int is_name_available(char *name) {
  int i;
  for(i = 0; i < clients_number; i++) {
    if(strcmp(clients[i].name, name) == 0) {
      return 0;
    }
  }
  return 1;
}

int send_message(int fd, char *name, char *content, int msg_id) {
    message msg;

    memset(&msg, 0, sizeof(message));
    if (strncpy(msg.sender, name, NAME_LENGTH-1) == NULL) {
      return -1;
    }
    if (strncpy(msg.message, content, MSG_LENGTH-1) == NULL) {
      return -1;
    }
    msg.msg_id = msg_id;

    if (write(fd, &msg, sizeof(msg)) == -1) {
      return -1;
    }
    return 0;
}

int add_to_clients(int fd) {
  int i;
  for(i = 0; i<=clients_number; i++) {
    if(i < SOCKETS_NUMBER) {
      if(clients[i].socket == 0) {
        clients[i].socket = fd;
        printf("Client added at %d\n", i);
        i++;
        break;
      }
    }
    else {
      errno = EPERM;
      return -1;
    }
  }
  if(i == clients_number+1) {
    clients_number++;
  }
  printf("Clients number: %d\n", clients_number);
  return 0;
}

void *network_thread_handler(void *arg) {
  max_fd = (socket_net>socket_loc) ? socket_net + 1 : socket_loc + 1;
  message msg;

  for(int i = 0; i<SOCKETS_NUMBER; i++) {
    clients[i].socket = 0;
    memset(clients[i].name, '\0', NAME_LENGTH);
  }

  while(1) {
    FD_ZERO(&set);
    FD_SET(socket_loc, &set);
    FD_SET(socket_net, &set);

    for(int i = 0; i < clients_number; i++) {
      FD_SET(clients[i].socket, &set);
    }

    if(select(max_fd, &set, NULL, NULL, NULL) < 0) {
      perror("Select error");
      exit(-1);
    }

    if(FD_ISSET(socket_loc, &set)) {
      int new_fd;
      struct sockaddr_un address;
      socklen_t addrlen = sizeof(address);
      if((new_fd = accept(socket_loc, (struct sockaddr*)&address, &addrlen)) < 0) {
        perror("Accept error");
        exit(-1);
      }
      if(max_fd < new_fd + 1) {
        max_fd = new_fd + 1;
      }
      if(add_to_clients(new_fd) == -1) {
        if(send_message(new_fd, SERVER_NAME, "Server is full", -1) == -1) {
          perror("Cannot send message");
          exit(-1);
        }
      }

      printf("New local connection:\nFD: %d\n\n" , new_fd);
    }

    if(FD_ISSET(socket_net, &set)) {
      int new_fd;
      struct sockaddr_in address;
      socklen_t addrlen = sizeof(address);
      if((new_fd = accept(socket_net, (struct sockaddr*)&address, &addrlen)) < 0) {
        perror("Accept error");
        exit(-1);
      }
      if(max_fd < new_fd + 1) {
        max_fd = new_fd + 1;
      }
      if(add_to_clients(new_fd) == -1) {
        if(send_message(new_fd, SERVER_NAME, "Server is full", -1) == -1) {
          perror("Cannot send message");
          exit(-1);
        }
      }
      printf("New IPv4 connection:\nFD: %d\nIP: %s\nPORT: %d\n\n" , new_fd , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));
    }

    for (int i = 0; i < clients_number; i++) {
      if (FD_ISSET(clients[i].socket, &set)) {
        memset(&msg, 0, sizeof(message));
        if(read(clients[i].socket, &msg, sizeof(message)) != 0) {
          if(msg.msg_id == 0) {
            //Greeting
            printf("Greeting message received\n");
            if(is_name_available(msg.sender)) {
              strcpy(clients[i].name, msg.sender);
              if(send_message(clients[i].socket, SERVER_NAME, "Welcome!", 0) == -1) {
                perror("Cannot send message");
                exit(-1);
              }
              printf("%s added to clients list\n", clients[i].name);
            }
            else {
              if(send_message(clients[i].socket, SERVER_NAME, "The name is not available!", -1) == -1) {
                perror("Cannot send message");
                exit(-1);
              }
              FD_CLR(clients[i].socket, &set);
              clients[i].socket = 0;

            }
          }
          else {
            if(msg.msg_id == -2) {
              printf("Divide by 0 error!\n");
            }
            else {
              if(msg.msg_id == -3) {
                FD_CLR(clients[i].socket, &set);
                clients[i].socket = 0;
                printf("Client %s left server\n", clients[i].name);
                fflush(stdin);
              }
              else {
                if(msg.msg_id > 0) {
                  printf("Order no. %d, result: %d from %s\n", msg.msg_id, msg.result, msg.sender);

                }
              }
            }
          }
        }
      }
    }
  }
}

int send_order(int fd, order_type order, int arg_1, int arg_2) {
  message msg;

  memset(&msg, 0, sizeof(message));
  if (strncpy(msg.sender, SERVER_NAME, NAME_LENGTH-1) == NULL) {
    return -1;
  }

  msg.msg_id = ++order_count;
  msg.type = order;
  msg.arg_1 = arg_1;
  msg.arg_2 = arg_2;

  if (write(fd, &msg, sizeof(msg)) == -1) {
    return -1;
  }
  printf("Order no. %d sent\n", msg.msg_id);
  return 0;
}
void write_out_menu() {
  printf("\n-----OPERATIONS-----\n[1] ADD\n[2] SUBTRACT\n[3] MULTIPLY\n[4] DIVIDE\n--------------------\n[5] EXIT\n\n");
}

int random_int(int from, int to) {
  return rand()%(to-from+1)+from;
}

Client randomize_client() {
  int i = random_int(0, clients_number);
  while(clients[i].socket == 0) {
    i = random_int(0, clients_number);
  }
  return clients[i];
}

void *terminal_thread_handler(void *arg) {
  int option;

  write_out_menu();
  while(1) {
    scanf("%d",&option);
    fflush(stdin);

    order_type order;
    int arg_1 = 0;
    int arg_2 = 0;

    switch(option) {
      case 1:
        order = ADD;
        break;
      case 2:
        order = SUBTRACT;
        break;
      case 3:
        order = MULTIPLY;
        break;
      case 4:
        order = DIVIDE;
        break;
      case 5:
        exit(0);
        break;
    }

    printf("First argument: ");
    scanf("%d", &arg_1);
    printf("Second argument: ");
    scanf("%d", &arg_2);
    send_order(randomize_client().socket, order, arg_1, arg_2);
    write_out_menu();
  }

}

void *clients_update_thread_handler(void *arg) {
  sigset_t sigset;
  sigemptyset(&sigset);
  sigaddset(&sigset, SIGPIPE);
  sigprocmask(SIG_BLOCK, &sigset, NULL);
  while(1) {
    sleep(10);
    for(int i = 0; i<clients_number; i++) {
      if(clients[i].socket != 0) {
        if(send_message(clients[i].socket, SERVER_NAME, "PING!", -2) == -1) {
          FD_CLR(clients[i].socket, &set);
          clients[i].socket = 0;
          printf("Client with name: %s is not responding, deleted from server\n", clients[i].name);
          fflush(stdin);

        }
      }
    }
  }
}

int main(int argc, char **argv) {
  printf("Server started\n");
  srand(time(NULL));
  if(argc < 3) {
    errno = EINVAL;
    perror("./server <port> <socket-path>");
    exit(-1);
  }

  socket_path = argv[2];
  const int PORT = atoi(argv[1]);

  signal(SIGINT, sigint_handler);
  if(atexit(exit_handler) == -1) {
    perror("Cannot define exit handler");
    exit(-1);
  }

  create_local_socket(socket_path);
  create_net_socket(PORT);

  pthread_t threads[3];
  if(pthread_create(&threads[0], NULL, network_thread_handler, NULL) == -1) {
    perror("Cannot create thread");
    exit(-1);
  }

  if(pthread_create(&threads[1], NULL, terminal_thread_handler, NULL) == -1) {
    perror("Cannot create thread");
    exit(-1);
  }

  if(pthread_create(&threads[1], NULL, clients_update_thread_handler, NULL) == -1) {
    perror("Cannot create thread");
    exit(-1);
  }

  for(int i = 0; i < 3; i++) {
    if(pthread_join(threads[i], NULL) != 0) {
      perror("Cannot join thread");
      exit(1);
    }
  }

  return 0;
}
