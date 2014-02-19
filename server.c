#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
//#include "structure.h"
#include "signal.h"
#include "message.h"
#include "server.h"

typedef struct _ip{
	unsigned char a;
	unsigned char b;
	unsigned char c;
	unsigned char d;
}IP;

typedef struct _accepted{
	int * sd;
	int * client_sd;
}ACCEPT_ARG;

char *getClientAddr(struct sockaddr_in * client_addr){
	int ipadd = client_addr->sin_addr.s_addr;
	IP *ipaddr_client = (struct _ip *) &ipadd;
	static char ip[16];
	sprintf(ip, "%d.%d.%d.%d",ipaddr_client->a, ipaddr_client->b, ipaddr_client->c, ipaddr_client->d);
	return (char *) &ip;
}

void *accepted(void *csd){
	int client_sd = *(int *)csd;
	while(1){
		char buff[100];
		int len;
        printf("BEFORE RECV\n");
		if((len=recv(client_sd,buff,sizeof(buff),0))<=0){
			if(errno == 0){
				printf("Client disconnected!\n");
				break;
			}
			printf("receive error: %s (Errno:%d)\n", strerror(errno),errno);
			exit(0);
		}
        printf("AFTER RECV\n");
		buff[len]='\0';
		printf("RECEIVED INFO: ");
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

		printf("Connected:\nClient Address: %s\n", getClientAddr(&client_addr));
		pthread_create(&pth, NULL, accepted, (void *) &client_sd);
	}
	pthread_join(pth,NULL);

	close(sd);
    return 0;
}
