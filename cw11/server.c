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

int socket_net;
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

void *network_thread_handler(void *arg) {

	struct sockaddr_in from;
	socklen_t addrlen;
	char buffer[128];

	while(1) {

   		if(recvfrom(socket_net, buffer, 128*sizeof(char), 0, (struct sockaddr*)&from, &addrlen) < 0) {
        		perror("Recvfrom error\n");
        		exit(-1);
      		}

	      	printf("BUFFER: %s", buffer);
	}

}

int main(int argc, char **argv) {

	create_net_socket(atoi(argv[1]));

	/*pthread_t thread;
  	if(pthread_create(&thread, NULL, network_thread_handler, NULL) == -1) {
    		perror("Cannot create thread");
    		exit(-1);
  	}

    	if(pthread_join(thread, NULL) != 0) {
      		perror("Cannot join thread");
      		exit(1);
    	}*/

  struct sockaddr_in from;
	socklen_t addrlen = sizeof(from);
	char buffer[128];

	while(1) {

		fflush(stdout);
   		if(recvfrom(socket_net, buffer, 128*sizeof(char), 0, (struct sockaddr*)&from, &addrlen) < 0) {
        		perror("Recvfrom error\n");
        		exit(-1);
      		}

	      	printf("BUFFER: %s\n", buffer);
	      	pid_t pid = fork();
	      	if(pid == 0) {
	      		if(strcmp(buffer, "Hello") == 0) {

	      			if (sendto(socket_net, "Hello", 128*sizeof(char), 0, (struct sockaddr*)&from, addrlen) == -1) {
      					perror("Cannot send message");
      					exit(-1);
    				}

	      			if(recvfrom(socket_net, buffer, 128*sizeof(char), 0, (struct sockaddr*)&from, &addrlen) < 0) {
        				perror("Recvfrom error\n");
        				exit(-1);
        			}
        			fflush(stdout);
        			printf("BUFFER: %s\n", buffer);

        			while(strcmp(buffer, "Bye") != 0) {
        				if(recvfrom(socket_net, buffer, 128*sizeof(char), 0, (struct sockaddr*)&from, &addrlen) < 0) {
        					perror("Recvfrom error\n");
        					exit(-1);
        				}
        			}

        			if (sendto(socket_net, "Bye", 128*sizeof(char), 0, (struct sockaddr*)&from, addrlen) == -1) {
      					perror("Cannot send message");
      					exit(-1);
    				}
       				exit(-1);
	      		}
	      		else
	      			exit(-1);
      		}

	}


	return 0;
}
