#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include "signal.h"
#include "message.h"
#include "server.h"

ARG *peers[10];


int online(ARG *arg){
	int i;
	for(i=0; i<10; i++){
		if(peers[i]!=NULL && strncmp(peers[i]->name, arg->name, arg->nameLen)==0)
			return -2;
		if(peers[i]!=NULL && (peers[i]->ip == arg->ip && peers[i]->port == arg->port))
			return -3;
	}
	for(i=0; i<10; i++){
		if(peers[i]==NULL){
			peers[i] = malloc(sizeof(ARG));
			memcpy(peers[i], arg, sizeof(ARG));
			return i;
		}
	}
	return -1;
}

void offline(int id){
	freeArg(peers[id]);
	peers[id] = NULL;
}

void printOnlineList(){
	int i;
	for(i = 0; i < 10; i++){
		printf("Printing %d\n", i);
		if(peers[i]!=NULL){
			printf("ID(%d)ONLINE: IP: %x Port: %d Name: %s\n", i, peers[i]->ip,peers[i]->port, peers[i]->name);
		}else{
			printf("ID(%d)OFFLINE\n", i);
		}
	}
}

void *accepted(void *csd){
	PEER *p = (PEER *) csd;
	int client_sd = p->client_sd;
	char *buff = malloc(2656);
	int len, id;
    printf("[SERVER]BEFORE RECV\n");
	
	if((len=recv(client_sd,buff,2656,0))<=0){
		if(errno == 0){
			printf("[SERVER]Client %s disconnected!\n", getClientAddr(&p->client_addr));
		}
		printf("[SERVER]receive error: %s (Errno:%d)\n", strerror(errno),errno);
		return;
	}

	DATA *d = parseData(buff);
	DATA *reply;
	DATA *err;
    switch(d->command){
    	case LOGIN:
    		printf("[SERVER] %s: LOGIN\n", getClientAddr(&p->client_addr));
			d = parseData(buff);
    		printf("[SERVER] %s: Username=%s Port=%d\n", 
    			getClientAddr(&p->client_addr),
    			d->arg->name,
    			d->arg->port);
    		d->arg->ip = ntohl(p->client_addr.sin_addr.s_addr);
    		int id = online(d->arg);
    		if(id >= 0){
    			peers[id]->ip = ntohl(p->client_addr.sin_addr.s_addr);
    			reply = newHeader();
    			reply->command = LOGIN_OK;
				toData(reply, buff);
    			if((len=send(client_sd,buff,1,0))<0){
    				printf("Reply Error: %s (Errno:%d)\n",strerror(errno),errno);
    			}
    		}else{
    			err = newHeader();
    			err->command = ERROR;
    			switch(id){
    				case -1:
    					err->error = TOO_MUCH_CONN;
    					break;
    				case -2:
    					err->error = SAME_NAME;
    					break;
    				case -3:
    					err->error = SAME_CONN;
    			}
    			toData(err, buff);
    			if((len=send(client_sd,buff,7,0))<0){
					printf("Reply Error: %s (Errno:%d)\n",strerror(errno),errno);
				}
    		}
    		break;
    	case GET_LIST:
    		printf("[SERVER] %s: GET_LIST\n", getClientAddr(&p->client_addr));
    		break;
    	default:
    		printf("[SERVER] %s: Unknown command\n", getClientAddr(&p->client_addr));
    		break;
    }


	buff[len]='\0';
	printf("[SERVER]RECEIVED INFO: ");
	if(strlen(buff)!=0)printf("%s\n",buff);
	if(strcmp("exit",buff)==0){
		close(client_sd);
		printf("[SERVER]Client %s disconnected!\n", getClientAddr(&p->client_addr));
	}
}

int main(int argc, char **argv){
    int sd = socket(AF_INET, SOCK_STREAM, 0);
    int client_sd;
    int addr_len;
    int len;
    int i;

    pthread_t pth;

	struct sockaddr_in server_addr;
	struct sockaddr_in client_addr;
	memset(&server_addr,0,sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(PORT);

	for(i=0;i<10;i++){
		peers[i] = NULL;
	}

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
		PEER passing;
		passing.client_sd = client_sd;
		passing.sd = sd;
		passing.client_addr = client_addr;
		pthread_create(&pth, NULL, accepted, (void *) &passing);
	}
	pthread_join(pth,NULL);

	close(sd);
    return 0;
}
