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

#define LOC_LIMIT 10
#define NET_LIMIT 10
#define NAME_LENGTH 16
#define MSG_LENGTH 200
#define SERVER_NAME "Krzysztof"

typedef enum order_type {
  ADD,
  SUBTRACT,
  DIVIDE,
  MULTIPLY
} order_type;

typedef struct client_net {
  char name[NAME_LENGTH];
  int is_deleted;
  struct sockaddr_in address;
  socklen_t addrlen;
} client_net;

typedef struct client_loc {
  char name[NAME_LENGTH];
  int is_deleted;
  struct sockaddr_un address;
  socklen_t addrlen;
} client_loc;

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
client_net clients_net[NET_LIMIT];
client_loc clients_loc[LOC_LIMIT];
int loc_count = 0;
int net_count = 0;
int order_count = 0;
fd_set set;


void create_net_socket(int port) {

    struct sockaddr_in addr;

    if ((socket_net = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
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
}

void create_local_socket(char* path) {

    struct sockaddr_un addr;

    if ((socket_loc = socket(AF_UNIX, SOCK_DGRAM, 0)) == -1) {
      perror("Cannot get socket descriptor");
      exit(-1);
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);
    addr.sun_path[sizeof(addr.sun_path) - 1] = '\0';

    unlink(socket_path);
    if (bind(socket_loc, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
      perror("Cannot bind socket");
      exit(-1);
    }
}

void sigint_handler(int signum) {
  exit(0);
}

void exit_handler() {

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
  for(i = 0; i < loc_count; i++) {
    if(strcmp(clients_loc[i].name, name) == 0) {
      return 0;
    }
  }

  for(i = 0; i < net_count; i++) {
    if(strcmp(clients_net[i].name, name) == 0) {
      return 0;
    }
  }

  return 1;
}

client_loc *get_loc_client_with_name(char *name) {
  for(int i = 0; i<loc_count; i++) {
    if(strcmp(clients_loc[i].name, name) == 0) {
      return &clients_loc[i];
    }
  }
  return NULL;
}

client_net *get_net_client_with_name(char *name) {
  for(int i = 0; i<net_count; i++) {
    if(strcmp(clients_net[i].name, name) == 0) {
      return &clients_net[i];
    }
  }
  return NULL;
}

int send_message(char *name, char *content, int msg_id, struct sockaddr *addr, socklen_t addrlen, int net) {
    message msg;

    memset(&msg, 0, sizeof(message));
    if (strncpy(msg.sender, name, NAME_LENGTH-1) == NULL) {
      return -1;
    }
    if (strncpy(msg.message, content, MSG_LENGTH-1) == NULL) {
      return -1;
    }
    msg.msg_id = msg_id;

    if (sendto(net ? socket_net : socket_loc, &msg, sizeof(msg), 0, addr, addrlen) == -1) {
      return -1;
    }
    return 0;
}

int add_to_clients(struct sockaddr_un *addr_loc, struct sockaddr_in *addr_net, socklen_t *addrlen, message *msg) {
  if(addr_loc != NULL && addrlen != NULL) {

    if(loc_count == LOC_LIMIT) {
      return -1;
    }

    int i = 0;
    for(int i = 0; i < loc_count; i++) {
      if(clients_loc[i].is_deleted) {
        break;
      }
    }
    if(is_name_available(msg->sender)) {
      strcpy(clients_loc[i].name, msg->sender);
      clients_loc[i].address = *addr_loc;
      clients_loc[i].addrlen = *addrlen;
      loc_count++;
      return 0;
    }
    else {
      return -1;
    }
  }

  else {
    if(addr_net != NULL && addrlen != NULL) {
      if(net_count == NET_LIMIT) {
        return -1;
      }

      int i = 0;
      for(int i = 0; i < net_count; i++) {
        if(clients_net[i].is_deleted) {
          break;
        }
      }
      if(is_name_available(msg->sender)) {
        strcpy(clients_net[i].name, msg->sender);
        clients_net[i].address = *addr_net;
        clients_net[i].addrlen = *addrlen;
        net_count++;
        return 0;
      }
      else {
        return -1;
      }
    }
    else {
      errno = EINVAL;
      return -1;
    }
  }

}

void *network_thread_handler(void *arg) {
  max_fd = (socket_net>socket_loc) ? socket_net + 1 : socket_loc + 1;

  for(int i = 0; i<LOC_LIMIT; i++) {
    clients_loc[i].is_deleted = 0;
    memset(clients_loc[i].name, '\0', NAME_LENGTH);
  }

  for(int i = 0; i<NET_LIMIT; i++) {
    clients_net[i].is_deleted = 0;
    memset(clients_net[i].name, '\0', NAME_LENGTH);
  }

  while(1) {
    message msg;
    memset(&msg, 0, sizeof(msg));

    FD_ZERO(&set);
    FD_SET(socket_loc, &set);
    FD_SET(socket_net, &set);

    if(select(max_fd, &set, NULL, NULL, NULL) < 0) {
      perror("Select error");
      exit(-1);
    }

    if(FD_ISSET(socket_loc, &set)) {
      struct sockaddr_un from;
      socklen_t addrlen = sizeof(from);

      if(recvfrom(socket_loc, &msg, sizeof(msg), 0, (struct sockaddr*)&from, &addrlen) < 0) {
        perror("Recvfrom error\n");
        exit(-1);
      }
      if(msg.msg_id == 0) {
          if(add_to_clients(&from, NULL, &addrlen, &msg) == -1) {
            if(send_message(SERVER_NAME, "Cannot add to clients", -1, (struct sockaddr*)&from, addrlen, 0) == -1) {
              perror("Error sending message");
              exit(-1);
            }
          } else {
            if(send_message(SERVER_NAME, "You have been sucessfully added to clients list", 0, (struct sockaddr*)&from, addrlen, 0) == -1) {
              perror("Error sending message");
              exit(-1);
            }
            printf("New local connection: %s\n" , msg.sender);
            printf("Loc clients count: %d\n\n", loc_count);
          }
      }
      else {
        if(msg.msg_id == -3) {
          client_loc *client = get_loc_client_with_name(msg.sender);
          if(client != NULL) {
            client->is_deleted = 1;
            printf("Client %s left server\n", client->name);
            fflush(stdin);
          }
        }
        else {
          if(msg.msg_id > 0) {
            printf("Order no. %d, result: %d from %s\n", msg.msg_id, msg.result, msg.sender);
          }
          else {
            if(msg.msg_id == -2) {
              printf("Divide by 0 error!\n");
            }
          }
        }
      }
    }

    if(FD_ISSET(socket_net, &set)) {
      struct sockaddr_in from;
      socklen_t addrlen = sizeof(from);
      message msg;
      memset(&msg, 0, sizeof(msg));

      if(recvfrom(socket_net, &msg, sizeof(msg), 0, (struct sockaddr*)&from, &addrlen) < 0) {
        perror("Recvfrom error\n");
        exit(-1);
      }

      if(msg.msg_id == 0) {
          if(add_to_clients(NULL, &from, &addrlen, &msg) == -1) {
            if(send_message(SERVER_NAME, "Cannot add to clients", -1, (struct sockaddr*)&from, addrlen, 1) == -1) {
              perror("Error sending message");
              exit(-1);
            }
          } else {
            if(send_message(SERVER_NAME, "You have been sucessfully added to clients list", 0, (struct sockaddr*)&from, addrlen, 1) == -1) {
              perror("Error sending message");
              exit(-1);
            }
            printf("New IPv4 connection: %s\n" , msg.sender);
            printf("Net clients count: %d\n\n", net_count);
          }
      }
      else {
        if(msg.msg_id == -3) {
          client_net *client = get_net_client_with_name(msg.sender);
          if(client != NULL) {
            client->is_deleted = 1;
            printf("Client %s left server\n", client->name);
            fflush(stdin);
          }
        }
        else {
          if(msg.msg_id > 0) {
            printf("Order no. %d, result: %d from %s\n", msg.msg_id, msg.result, msg.sender);
          }
          else {
            if(msg.msg_id == -2) {
              printf("Divide by 0 error!\n");
            }
          }
        }
      }
    }
  }
}

int send_order(order_type order, int arg_1, int arg_2, struct sockaddr *addr, socklen_t addrlen, int net) {
  message msg;

  memset(&msg, 0, sizeof(message));
  if (strncpy(msg.sender, SERVER_NAME, NAME_LENGTH-1) == NULL) {
    return -1;
  }

  msg.msg_id = ++order_count;
  msg.type = order;
  msg.arg_1 = arg_1;
  msg.arg_2 = arg_2;

  if (sendto(net ? socket_net : socket_loc, &msg, sizeof(msg), 0, addr, addrlen) == -1) {
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

client_net *randomize_net_client() {
  if(net_count == 0) return NULL;
  int i = random_int(0, net_count-1);
  while(clients_net[i].is_deleted) {
    i = random_int(0, net_count-1);
  }

  return &clients_net[i];
}

client_loc *randomize_loc_client() {
  if(loc_count == 0) return NULL;
  int i = random_int(0, loc_count-1);
  while(clients_loc[i].is_deleted) {
    i = random_int(0, loc_count-1);
  }
  return &clients_loc[i];
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

    int is_net;
    if(net_count == 0 && loc_count == 0) {
      printf("There's no available client\n");
    }

    if(net_count>0 && loc_count>0) {
      is_net = random_int(0,1);
    }
    else {
      if(net_count>0) is_net = 1;
      else is_net = 0;
    }

    if(is_net) {
      client_net *client = randomize_net_client();
      send_order(order, arg_1, arg_2, (struct sockaddr*)&(client->address), client->addrlen, is_net);
    }
    else {
      client_loc *client = randomize_loc_client();
      send_order(order, arg_1, arg_2, (struct sockaddr*)&(client->address), client->addrlen, is_net);
    }

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
    for(int i = 0; i<loc_count; i++) {
      if(!clients_loc[i].is_deleted) {
        if(send_message(SERVER_NAME, "PING!", -2, (struct sockaddr *)&(clients_loc[i].address), clients_loc[i].addrlen, 0) == -1) {
          clients_loc[i].is_deleted = 1;
          printf("Client with name: %s is not responding, deleted from server\n", clients_loc[i].name);
          fflush(stdin);

        }
      }
    }
    for(int i = 0; i<net_count; i++) {
      if(!clients_net[i].is_deleted) {
        if(send_message(SERVER_NAME, "PING!", -2, (struct sockaddr *)&(clients_net[i].address), clients_net[i].addrlen, 1) == -1) {
          clients_net[i].is_deleted = 1;
          printf("Client with name: %s is not responding, deleted from server\n", clients_net[i].name);
          fflush(stdin);

        }
      }
    }
  }
return 0;
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
  printf("Local socket created\n");
  create_net_socket(PORT);
  printf("IPv4 socket created\n");

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
