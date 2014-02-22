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
	printf("Clearing %d\n", id);
	freeArg(peers[id]);
	peers[id] = NULL;
}

void printOnlineList(){
	int i;
	printf("Listing online clients: \n");
	for(i = 0; i < 10; i++){
		if(peers[i]!=NULL){
			printf("ID(%d)ONLINE: IP: %x Port: %d Name: %s\n", i, peers[i]->ip,peers[i]->port, peers[i]->name);
		}
	}
}

void *accepted(void *csd){
	PEER *p = (PEER *) csd;
	DATA *data = malloc(sizeof(DATA));
	DATA *reply = malloc(sizeof(DATA));
	int client_sd = p->client_sd;
	char *buff = malloc(2656);
	int len, id, status;
	
	while(1){
		data = recv_data(client_sd,&len,&status);
		switch(status){
			case -1:
			case -2:
				close(client_sd);
				printf("closed client conn\n");
				offline(id);
				printf("removed online record\n");
				printOnlineList();
				printf("[SERVER]Client %s disconnected!\n", getClientAddr(&p->client_addr));
				return;
			default:
				break;
		}

	    switch(data->command){
	    	case LOGIN:
	    		printf("[SERVER] %s: LOGIN\n", getClientAddr(&p->client_addr));
	    		printf("[SERVER] %s: Username=%s Port=%d\n", 
	    			getClientAddr(&p->client_addr),
	    			data->arg->name,
	    			data->arg->port);
	    		data->arg->ip = ntohl(p->client_addr.sin_addr.s_addr);
	    		id = online(data->arg);
	    		printf("[SERVER] %s: LOGGED IN, id=%d\n", getClientAddr(&p->client_addr), id);
	    		if(id >= 0){
	    			reply = newHeader();
	    			reply->command = LOGIN_OK;
	    			reply->length = 1;
					if(!send_data(client_sd,reply,&len)) return;
	    		}else{
	    			reply = newHeader();
	    			reply->command = ERROR;
	    			switch(id){
	    				case -1:
	    					reply->error = TOO_MUCH_CONN;
	    					break;
	    				case -2:
	    					reply->error = SAME_NAME;
	    					break;
	    				case -3:
	    					reply->error = SAME_CONN;
	    			}
	    			reply->length = 7;
	    			if(!send_data(client_sd,reply,&len)) return;
	    			close(client_sd);
	    			printOnlineList();
					printf("[SERVER]Client %s disconnected!\n", getClientAddr(&p->client_addr));
					return;
	    		}
	    		break;
	    	case GET_LIST:
	    		printf("[SERVER] %s: GET_LIST\nCurrent online users:\n", getClientAddr(&p->client_addr));
	    		printOnlineList();
	    		break;
	    	default:
	    		printf("[SERVER] %s: Unknown command\n", getClientAddr(&p->client_addr));
	    		break;
	    }
	}

	close(client_sd);
	offline(id);
	printOnlineList();
	printf("[SERVER]Client %s disconnected!\n", getClientAddr(&p->client_addr));

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

		printf("[SERVER]Connected:\nClient Address: %s Port: %d\n", 
			getClientAddr(&client_addr), 
			htons(client_addr.sin_port));
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
