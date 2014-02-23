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
PEER passing[10];

int online(ARG *arg, int id){
	int i;
	for(i=0; i<10; i++){
		if(peers[i]==NULL) continue;
		if(peers[i]->nameLen == arg->nameLen){
			if(strncmp(peers[i]->name, arg->name, arg->nameLen)==0)
				return -2;
		}
		if(peers[i]->ip == arg->ip && peers[i]->port == arg->port)
			return -3;
	}
	if(peers[id]==NULL){
		peers[id] = malloc(sizeof(ARG));
		memcpy(peers[id], arg, sizeof(ARG));
		return id;
	}
	return -1;
}

void offline(int id){
	freeArg(peers[id]);
	peers[id] = NULL;
}

void printName(char * name, int len){
	int i;
	for(i=0; i<len;i++){
		printf("%c", name[i]);
	}
}

void printOnlineList(){
	int i;
	printf("Listing online clients: \n");
	for(i = 0; i < 10; i++){
		if(peers[i]!=NULL){
			printf("ID(%d)ONLINE: IP: %lx Port: %d Name: %s\n", i, peers[i]->ip,peers[i]->port, peers[i]->name);
		}
	}
}

void getOnlineList(DATA *data){
	int i;
	for(i=0;i<10;i++){
		if(peers[i]!=NULL){
			addClient(data, newClient(
					peers[i]->name,
					peers[i]->ip,
					peers[i]->port,
					peers[i]->nameLen
				));
			data->length += peers[i]->nameLen;
			data->length += (4+4+2);
		}
	}
}

void printClientList(ARG * arg){
	if(arg!=NULL){
		printf("Client: %s\n", arg->name);
		printClientList(arg->arg);
	}
}

void *accepted(void *csd){
	PEER *p = (PEER *) csd;
	DATA *data = malloc(sizeof(DATA));
	DATA *reply = malloc(sizeof(DATA));
	int client_sd = p->client_sd;
	char *buff = malloc(2656);
	int len, status;
	int id = p->id;
	
	while(1){
		data = recv_data(client_sd,&len,&status);
		switch(status){
			case -1:
			case -2:
				close(client_sd);
				offline(id);
				printf("[Disconnected] Client %s:%d\n", 
					getClientAddr(&p->client_addr),
					ntohs(p->client_addr.sin_port));
				return;
			default:
				break;
		}

	    switch(data->command){
	    	case LOGIN:
	    		printf("Client %s:%d: [LOGIN] Username=", 
	    			getClientAddr(&p->client_addr),
	    			ntohs(p->client_addr.sin_port));
	    		printName(data->arg->name, data->arg->nameLen);
	    		printf("\n");
	    		data->arg->ip = ntohl(p->client_addr.sin_addr.s_addr);
	    		status = online(data->arg, id);
	    		if(status >= 0 && status < 10){
	    			reply = newHeader();
	    			reply->command = LOGIN_OK;
	    			reply->length = 1;
					if(!send_data(client_sd,reply,&len)) return;
	    		}else{
	    			printf("[ERROR] Client %s:%d ", 
						getClientAddr(&p->client_addr),
						ntohs(p->client_addr.sin_port));
	    			reply = newHeader();
	    			reply->command = ERROR;
	    			switch(status){
	    				case -1:
	    					printf("maximum connection exceed!");
	    					reply->error = TOO_MUCH_CONN;
	    					break;
	    				case -2:
	    					printf("duplication of name!");
	    					reply->error = SAME_NAME;
	    					break;
	    				case -3:
	    					printf("duplication of port and IP address!");
	    					reply->error = SAME_CONN;
	    			}
	    			printf("\n");
	    			reply->length = 7;
	    			if(!send_data(client_sd,reply,&len)) return;
	    			close(client_sd);
					printf("[Disconnected] Client %s:%d\n", 
						getClientAddr(&p->client_addr),
						ntohs(p->client_addr.sin_port));
					return;
	    		}
	    		break;
	    	case GET_LIST:
	    		printf("Client %s:%d: GET_LIST\n", 
	    			getClientAddr(&p->client_addr),
	    			ntohs(p->client_addr.sin_port));
	    		reply = newHeader();
	    		reply->command = GET_LIST_OK;
	    		getOnlineList(reply);
	    		reply->length += 7;
				if(!send_data(client_sd,reply,&len)) return;
	    		break;
	    	default:
	    		printf("Client %s:%d: Unknown command\n", 
	    			getClientAddr(&p->client_addr),
	    			ntohs(p->client_addr.sin_port));
	    		break;
	    }
	}

	close(client_sd);
	offline(id);
	printOnlineList();
	printf("[Disconnected] Client %s:%d\n", 
		getClientAddr(&p->client_addr),
		p->client_addr.sin_port);

}

int main(int argc, char **argv){
    int sd = socket(AF_INET, SOCK_STREAM, 0);
    int client_sd;
    int addr_len;
    int len;
    int i;

    pthread_t pth[10];

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

		printf("[Connected] Client: %s:%d\n", 
			getClientAddr(&client_addr), 
			htons(client_addr.sin_port));
		for(i=0;i<10;i++){
			if(peers[i]==NULL){
				passing[i].id = i;
				passing[i].client_sd = client_sd;
				passing[i].sd = sd;
				passing[i].client_addr = client_addr;
				pthread_create(&pth[i], NULL, accepted, (void *) &passing[i]);
				break;
			}
		}
		if(i==10){
			DATA *reply = newHeader();
			printf("[ERROR] Client %s:%d ", 
				getClientAddr(&client_addr),
				ntohs(client_addr.sin_port));
			reply = newHeader();
			reply->command = ERROR;
			printf("maximum connection exceed!");
			reply->error = TOO_MUCH_CONN;
			printf("\n");
			reply->length = 7;
			send_data(client_sd,reply,&len);
			close(client_sd);
			printf("[Disconnected] Client %s:%d\n", 
				getClientAddr(&client_addr),
				ntohs(client_addr.sin_port));
		}
		
	}
	for(i=0;i<10;i++)
		pthread_join(pth[i],NULL);

	close(sd);
    return 0;
}
