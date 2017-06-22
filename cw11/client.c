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

struct sockaddr *server;
int sock;
struct sockaddr_in net_addr;
void create_net_socket(char *address, int port) {



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

int main(int argc, char **argv) {

	create_net_socket(argv[1], atoi(argv[2]));
	
	char buffer[128];
	
	if (sendto(sock, "Hello", 128*sizeof(char), 0, server, sizeof(*server)) == -1) {
      		perror("Cannot send message");
      		exit(-1);
    	}
    	
    	struct sockaddr_in from;
	socklen_t addrlen = sizeof(from);
	
    	if(recvfrom(sock, buffer, 128*sizeof(char), 0, (struct sockaddr*)&from, &addrlen) < 0) {
        	perror("Recvfrom error\n");
        	exit(-1);
        }
        
        if(strcmp(buffer, "Hello") == 0) {
        	printf("Server: %s\n", buffer);
        	if (sendto(sock, "dane", 128*sizeof(char), 0, server, sizeof(*server)) == -1) {
	      		perror("Cannot send message");
	      		exit(-1);
    		}
    		
    		if (sendto(sock, "Bye", 128*sizeof(char), 0, server, sizeof(*server)) == -1) {
	      		perror("Cannot send message");
	      		exit(-1);
    		}
        }
        if(recvfrom(sock, buffer, 128*sizeof(char), 0, (struct sockaddr*)&from, &addrlen) < 0) {
        	perror("Recvfrom error\n");
        	exit(-1);
        }
        if(strcmp(buffer, "Bye") == 0) {
        	printf("Server: %s\n", buffer);
        }
    	

	return 0;
}