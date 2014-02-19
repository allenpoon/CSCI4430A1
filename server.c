#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include "structure.h"
#include "signal.h"
#include "message.h"
#include "server.h"

char *getClientAddr(struct sockaddr_in * client_addr){
	static char ip[19];
	sprintf(ip, "%d.%d.%d.%d",
		(int)(client_addr->sin_addr.s_addr & 0xff), 
		(int)((client_addr->sin_addr.s_addr & 0xff00)>>8), 
		(int)((client_addr->sin_addr.s_addr & 0xff0000)>>16), 
		(int)((client_addr->sin_addr.s_addr & 0xff000000)>>24));
	return (char *) &ip;
}

void *accepted(void *csd){
	int client_sd = *(int *)csd;
	while(1){
		char buff[100];
		int len;
        printf("[SERVER]BEFORE RECV\n");
		if((len=recv(client_sd,buff,sizeof(buff),0))<=0){
			if(errno == 0){
				printf("[SERVER]Client disconnected!\n");
				break;
			}
			printf("[SERVER]receive error: %s (Errno:%d)\n", strerror(errno),errno);
			exit(0);
		}
        printf("[SERVER]AFTER RECV\n");
		buff[len]='\0';
		printf("[SERVER]RECEIVED INFO: ");
		if(strlen(buff)!=0)printf("%s\n",buff);
		if(strcmp("exit",buff)==0){
			
			close(client_sd);
			break;
		}
	}
}

int main(int argc, char **argv){
    int sd = socket(AF_INET, SOCK_STREAM, 0);
    int client_sd;
    int addr_len;
    int len;

    pthread_t pth;

	struct sockaddr_in server_addr;
	struct sockaddr_in client_addr;
	memset(&server_addr,0,sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(PORT);

	if(bind(sd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0){
		printf("Socket cannot bind to a port. %s \nError number: %d\n", strerror(errno), errno);
	}

	if(listen(sd,3) < 0){
		printf("Cannot listen to a port. %s \nError number: %d\n", strerror(errno), errno);
	}

	addr_len = sizeof(client_addr);
	while(1){
		if((client_sd = (accept(sd, (struct sockaddr *) &client_addr, &addr_len))) < 0){
			printf("Connection error. Accepting client failed. %s \nError number: %d\n", strerror(errno), errno);
			exit(0);
		}

		printf("[SERVER]Connected:\nClient Address: %s Port: %d\n", getClientAddr(&client_addr), htons(client_addr.sin_port));
		pthread_create(&pth, NULL, accepted, (void *) &client_sd);
	}
	pthread_join(pth,NULL);

	close(sd);
    return 0;
}
