# include <stdio.h>
# include <stdlib.h>
# include <unistd.h>
# include <string.h>
# include <errno.h>
# include <sys/socket.h>
# include <sys/types.h>
#include <arpa/inet.h>
# include <netinet/in.h>
# include <pthread.h>
# include "message.h"
# include "signal.h"

#define NAME_LEN 255
#define MAX_MSG_LEN 255
#define BUFF_LEN 2656
#define MAX_CLIENT 10

# define IPADDR "127.0.0.1"
# define PORT 12310

void loginedMenu();
void closeAllConn();
void renewClientList();

// self info
// NAME_LEN + 1 end char + 1 check too long name char
char name[NAME_LEN+2];
char tmpName[NAME_LEN+2];
struct sockaddr_in server_addr;
struct sockaddr_in local_addr;
struct sockaddr_in tmp_addr;


// listening port info
short port_assigned = -1;
unsigned short port=0;


// 1 server + 10 client
// 0 == server, 1..10 == client
// MAX 9 Conn, 1 client is accept for kick it out
DATA *connInfo[MAX_CLIENT+1]; 
DATA *otherMsg=0;

// prevent passive and active client login in the same time
pthread_mutex_t conn_mutex = PTHREAD_MUTEX_INITIALIZER;
// waiting lock
pthread_mutex_t port_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t listen_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t active_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t recv_mutex = PTHREAD_MUTEX_INITIALIZER;

// 0 == listening, 1..10 == passive+active
pthread_t thread[MAX_CLIENT+1]; 

// 0 == listen, 1..10 == client
int socket_id[MAX_CLIENT+1];
int socket_serv;

// 0 == server, 1..10 == client
unsigned char sockBuff[MAX_CLIENT+1][BUFF_LEN];

// new active client data passing
char *newClientName;

void printName(char * name, int len){
    int i;
    for(i=0; i<len;i++){
        printf("%c", name[i]);
    }
}

void *clientRecver(void * id){
	int thread_id = *(int *) id;
	DATA *tmp;
    pthread_mutex_unlock(&listen_mutex);
    pthread_mutex_unlock(&active_mutex);
	while(1){
		tmp = recv_data_buff(socket_id[thread_id], 0, 0, sockBuff[thread_id]);

		if(tmp){
			if(tmp->command == MSG){
                pthread_mutex_lock(&recv_mutex);
				addArg(connInfo[thread_id], tmp->arg);
                pthread_mutex_unlock(&recv_mutex);
				tmp->arg=0;
			}else{
//				printf("Thread [%d]: Unknow Message Receive.\n",thread_id);
//				printf("Thread [%d]: command:%d.\n", thread_id, tmp->command);
				break;
			}
			freeData(tmp);
		}else{
//			printf("Thread [%d]: Disconnected.\n",thread_id);
			break;
		}
	}
	if(connInfo[thread_id] && connInfo[thread_id]->arg && connInfo[thread_id]->arg->arg){
		if(otherMsg){
			tmp =otherMsg;
			while(tmp->next){
				tmp=tmp->next;
			}
			tmp->next=connInfo[thread_id];
		}else{
			otherMsg=connInfo[thread_id];
		}
		connInfo[thread_id] = 0;
	}
	
	if(connInfo[thread_id]) freeData(connInfo[thread_id]);
	connInfo[thread_id]=0;
	if(socket_id[thread_id]) close(socket_id[thread_id]);
	socket_id[thread_id]=0;
	thread[thread_id]=0; // end of thread
	return 0;
}

// a active client thread
int activeClient(int thread_id){
	ARG *tmp = connInfo[0]->arg;
	int i;

	// finding requested client

	while(tmp && strcmp((char *)tmp->name, (char *)tmpName)){
		tmp = tmp->arg;
	}
	if(tmp){
		socket_id[thread_id]=socket(AF_INET,SOCK_STREAM,0);

		tmp_addr.sin_family=AF_INET;
		tmp_addr.sin_addr.s_addr=htonl(tmp->ip);
		tmp_addr.sin_port=htons(tmp->port);

		for(i=0;i<5 && connect(socket_id[thread_id],(struct sockaddr *)&tmp_addr,sizeof(tmp_addr))<0 ;i++){
			printf("connection error: %s (Errno:%d)\n",strerror(errno),errno);
			printf("Retry Connecting to [%s:%hd] ...\n", (char *) inet_ntoa(tmp_addr.sin_addr), tmp_addr.sin_port);
		}

		if(i==5){
	        printf("Cannot Connect to [%s:%hd].\n", (char *) inet_ntoa(tmp_addr.sin_addr), tmp_addr.sin_port);
	    }else{
			printf("Connecting to '%s' ... ", tmpName);
			connInfo[thread_id]=newHeader();
			connInfo[thread_id]->command = HELLO;
			addClient(connInfo[thread_id], newClient((unsigned char*)name, 0, 0, strlen(name)));
			send_data_buff(socket_id[thread_id], connInfo[thread_id], (unsigned int *)&i, sockBuff[thread_id]);
			freeData(connInfo[thread_id]);
			connInfo[thread_id]=recv_data_buff(socket_id[thread_id], (unsigned int *)&i, 0, sockBuff[thread_id]);
			
			switch(connInfo[thread_id]->command){
	            case HELLO_OK:
	                printf("Done\n");
					connInfo[thread_id]->arg = newClient((unsigned char *)tmpName, tmp_addr.sin_addr.s_addr, tmp_addr.sin_port, strlen(tmpName));
					pthread_mutex_lock(&active_mutex);
					pthread_create(&thread[thread_id], 0, clientRecver, (void *)&thread_id);
					pthread_mutex_lock(&active_mutex);
					pthread_mutex_unlock(&active_mutex);
					return 1;
	                break;
	            case ERROR:
				default:
	                if(connInfo[thread_id]->error == NOT_IN_LIST){
	                    printf("Sorry, You are not in Server List.\n");
	                }else{
	                	printf("Sorry, Unknowed error occur. (%d)\n", connInfo[thread_id]->error);
	                }
	                
	                if(connInfo[thread_id]) freeData(connInfo[thread_id]);
	                connInfo[thread_id]=0;
	                
	                if(socket_id[thread_id]) close(socket_id[thread_id]);
	                socket_id[thread_id] =0;
	                return 0;
	                break;
	        }
	    }
	}else{
		printf("User Not Found.\n");
	}
	return 0;
}

// a passive client thread
void passiveClient(int client_sd, unsigned long ip, unsigned short port){
	int i,j;
	DATA *tmp;
	ARG *tmpArg;
	pthread_mutex_lock(&conn_mutex);
    // finding available thread
    for(i=1;i<MAX_CLIENT+1 && thread[i]; i++);
    if(i>=MAX_CLIENT+1){
    	tmp = newHeader();
    	tmp->command = ERROR;
		tmp->error = TOO_MUCH_CONN;
		send_data(client_sd, tmp, NULL);
		freeData(tmp);
		close(client_sd);
    }else{
    	for(j=1;j<MAX_CLIENT+1 && (!connInfo[j] || (connInfo[j]->arg->ip ==ip && connInfo[j]->arg->port==port));j++);
    	if(j<MAX_CLIENT+1){ // same ip and port connection
			tmp=newHeader();
			tmp->command = ERROR;
			tmp->error = SAME_CONN;
			send_data(client_sd, tmp, 0);
			freeData(tmp);
			close(client_sd);
    	}else{
    		tmp = recv_data_buff(client_sd, 0,0,sockBuff[i]);
    		if(tmp && tmp->command == HELLO){
    			for(j=1;j<MAX_CLIENT+1 && (!connInfo[j] || strcmp((char *)connInfo[j]->arg->name,(char *)tmp->arg->name)); j++);
    			if(j<MAX_CLIENT+1){ // same client
					tmp=newHeader();
					tmp->command = ERROR;
					tmp->error = SAME_NAME;
					send_data_buff(client_sd, tmp, 0, sockBuff[i]);
					freeData(tmp);
					close(client_sd);
    			}else{
    				// basically accepted
    				// store Header
					connInfo[i] = tmp;
					printf("Name Receive: %s\n", tmp->arg->name);
					socket_id[i]=client_sd;
// need to store IP and port
					renewClientList();
    				// check list - is name exist?
    				tmpArg =connInfo[0]->arg;
					while(tmpArg && strcmp((char *)tmpArg->name, (char *)connInfo[i]->arg->name)){
						tmpArg = tmpArg->arg;
					}
					if(tmpArg){
						// connected
						tmp = newHeader();
						tmp->command=HELLO_OK;
						send_data_buff(client_sd, tmp, 0, sockBuff[i]);
						if(connInfo[i]) freeData(connInfo[i]);
						connInfo[i] = tmp;
						addArg(connInfo[i], newClient(
							tmpArg->name,
							ip,
							port,
							strlen((char *)tmpArg->name)+1));
    					pthread_mutex_lock(&listen_mutex);
    					// thread will unlock
						pthread_create(&thread[i], NULL, (void *) &clientRecver, &i);
    					pthread_mutex_lock(&listen_mutex);
    					pthread_mutex_unlock(&listen_mutex);
					}else{
						// connection refuse
						tmp->command = ERROR;
						tmp->error = NOT_IN_LIST;
						send_data_buff(client_sd, tmp, 0, sockBuff[i]);
						freeData(tmp);
						connInfo[i] = 0;
						close(client_sd);
					}
    			}
    		}else{
    			printf("Unkown Command Recive.\n");
    			close(client_sd);
    			if(tmp) freeData(tmp);
    		}
    	}
    }
    
    pthread_mutex_unlock(&conn_mutex);
}

void *listening(){
	int i=0;
	int client_sd;
	struct sockaddr_in client_addr;
    unsigned int addr_len;
	
    socket_id[0] = socket(AF_INET, SOCK_STREAM, 0);
    memset(&local_addr,0,sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    local_addr.sin_port = 0;

    printf("Opening an arbitrary listening port ... ");

    while(bind(socket_id[0], (struct sockaddr *) &local_addr, sizeof(local_addr)) < 0 && i<5){
        printf("\nSocket cannot bind to a port. %s", strerror(errno));
        printf("\nError number: %d\n", errno);
        if(i++<5) printf("\nRetrying Binding ... ");
    }
	if(i<5){
		if(listen(socket_id[0],MAX_CLIENT) < 0){
			printf("Cannot listen to a port. %s \nError number: %d\n", strerror(errno), errno);
		}
	    unsigned int addr_size = sizeof(local_addr);
	    getsockname(socket_id[0], (struct sockaddr *) &local_addr, &addr_size);
	    printf("done\n");
	    printf("Listening Port: %d\n", ntohs ( ((struct sockaddr_in *)&local_addr)->sin_port ));

	    port = ntohs ( ((struct sockaddr_in *)&local_addr)->sin_port );

	}else{
		port = 0;
	}
    pthread_mutex_unlock(&port_mutex);
    
//    start accept client
	if(port > 0){
		while(1){
			if((client_sd = accept(socket_id[0], (struct sockaddr *) &client_addr, &addr_len)) < 0){
				printf("Connection error. Accepting client failed. %s \nError number: %d\n", strerror(errno), errno);
				break; // end of listening
			}
			passiveClient(client_sd, client_addr.sin_addr.s_addr, client_addr.sin_port);
		}
	}

	// clear all memory, close all stream
	return 0;
}



void goOnline(){
    DATA *tmpData;
	int i, len;

    // start listen
    pthread_mutex_lock(&port_mutex);
    pthread_create(&thread[0], NULL, listening, NULL);
    
    // waiting for listen thread after binding port
    pthread_mutex_lock(&port_mutex);
    pthread_mutex_unlock(&port_mutex);

	if(!port){
		printf("We cannot bind a port for listening, Fail to online.\n");
	}else{
	    // connect to server
	    socket_serv = socket(AF_INET,SOCK_STREAM,0);

		for(i=0;i<5 && connect(socket_serv,(struct sockaddr *)&server_addr,sizeof(server_addr))<0 ;i++){
			printf("connection error: %s (Errno:%d)\n",strerror(errno),errno);
			printf("Retry Connecting to [%s:%hd] ...\n", getClientAddr(&server_addr), ntohs(server_addr.sin_port));
		}
		if(i==5){
	        printf("Cannot Connect to [%s:%hd].\n", getClientAddr(&server_addr), ntohs(server_addr.sin_port));
	    }else{
	        // connection completed
	        // ask for user action
	        printf("[ Logging in ");
							fflush(stdout);  	// --Just
	        tmpData = newHeader();
	        
	        				putchar('.'); 		// --for
							fflush(stdout);  	// --fun
	        tmpData->command = LOGIN;
	        addClient(tmpData, newClient((unsigned char *)name, 0, htonl(port), strlen(name)));
	        				putchar('.'); 		// --Just
							fflush(stdout);  	// --for
	        if(!send_data_buff(socket_serv,tmpData,(unsigned int*)&len, sockBuff[0])){
	        	freeData(tmpData);
	            //ERROR
	            printf("Oops, We cannot send data to server.\n");
	            printf("Going offline ...\n");
	            close(socket_serv);
	            return;
	        }
	        				putchar('.'); 		// --for
							fflush(stdout);  	// --fun

	        freeData(tmpData);
	        tmpData = recv_data(socket_serv,(unsigned int *)&len,&i);
	        				putchar('.'); 		// --Just
							fflush(stdout);  	// --for
	        if(!i){
	        	freeData(tmpData);
	            //ERROR
	            close(socket_serv );
	            return;
	        }
	        				putchar('.'); 		// --fun
							fflush(stdout);  	// --Just
	        switch(tmpData->command){
	            case LOGIN_OK:
	                printf("\"Hello, %s!\" ]\n", name); // --for fun; End of Just for fun
	                freeData(tmpData);
	                loginedMenu();
	                break;
	            case ERROR:
	                switch(tmpData->error){
	                    case SAME_NAME:
	                        printf("\"Sorry, '%s' is already there!\"\n",name);
	                        break;
	                    case SAME_CONN:
	                        printf("\"Sorry, there is same ip and port of client.\"\n");
	                        break;
	                    case TOO_MUCH_CONN:
	                        printf("\"Sorry, there are too much client.\"\n");
	                        break;
	                }
	                freeData(tmpData);
	                break;
	        }
	    }
    	close(socket_serv);
    }
// close all socket, close all thread, free memory
}


void readMsg(){
    int i,j,k=0;
    DATA *tmp;
    printf("\n");
	while(otherMsg && otherMsg->arg && otherMsg->arg->arg){
        j=0;
        pthread_mutex_lock(&recv_mutex);
        while(otherMsg->arg->arg){
            printf("+--- Message #%-2d From '%s'\n", ++j, otherMsg->arg->name);
            printf("%s", otherMsg->arg->arg->msg);
            printf("+--------------------------------\n\n");
// need to set mutex
            removeArg(otherMsg, otherMsg->arg->arg);
            k++;
        }
        pthread_mutex_unlock(&recv_mutex);
        tmp = otherMsg;
        otherMsg = otherMsg->next;
		freeData(tmp);
	}
    for(i=1;i<=MAX_CLIENT;i++){
        if(connInfo[i] && connInfo[i]->arg && connInfo[i]->arg->arg){
            j=0;
            pthread_mutex_lock(&recv_mutex);
            while(connInfo[i]->arg->arg){
                printf("+--- Message #%-2d From '%s'\n", ++j, connInfo[i]->arg->name);
                printf("%s", connInfo[i]->arg->arg->msg);
                printf("+--------------------------------\n\n");
// need to set mutex
                removeArg(connInfo[i], connInfo[i]->arg->arg);
                k++;
            }
            pthread_mutex_unlock(&recv_mutex);
        }
    }
    if(k==0){
        printf("[ No unread message. ]\n");
    }
    printf("\n");
}

void renewClientList(){
    DATA *tmpData;
	tmpData = newHeader();
    tmpData->command = GET_LIST;
    send_data_buff(socket_serv, tmpData, 0, sockBuff[0]);
    freeData(tmpData);
    tmpData=recv_data_buff(socket_serv, 0, 0, sockBuff[0]);
	if(connInfo[0]){
		freeData(connInfo[0]);
	}
	connInfo[0]=tmpData;
}

void showClientList(){
    ARG * tmpArg;
    int i;
    printf("Retrieving from server .... ");
    renewClientList();
    printf("Done.\n");
    if(connInfo[0]->command == GET_LIST_OK){
        tmpArg = connInfo[0]->arg;
        i=1;
		printf("\n");
		printf("+--- Online List ----------------\n");
		do{
		    printf("| %2d) ", i++);
            printName((char*)tmpArg->name, tmpArg->nameLen);
            printf("\n");
		    tmpArg=tmpArg->arg;
        }while(tmpArg);
		printf("+--------------------------------\n");
    }else{
        printf("Get List Error occur, unknown command id.");
    }
}

void startConn(){
    int i,counter,status=1;
    unsigned char *str = sockBuff[0];

// connection here
    
    for(i=1;i<MAX_CLIENT + 1 && !(connInfo[i] && !strcmp((char *)connInfo[i]->arg->name, (char *)tmpName));i++);

    if(i>MAX_CLIENT){
        pthread_mutex_lock(&conn_mutex);
        renewClientList();
        for(i=1;i<MAX_CLIENT + 1;i++){
            if(!thread[i]){
// create active client
                status = activeClient(i);
                break;
            }
        }
        pthread_mutex_unlock(&conn_mutex);
    }
    
    if(!status) return;
        // thread exist if [ 0 < i < MAX_CLIENT+1 ]
        if(i<MAX_CLIENT+1){
            counter =0;
            printf("\n");
            printf("+---Type your message to '%s'-------------------\n", tmpName);
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            do{
                fgets((char *)str+counter, MAX_MSG_LEN+2, stdin);
                if(!strcmp(".\n", (char *)str+counter)){
                    *(str+counter) = 0;
                    break;
                }
                counter += strlen((char *)str+counter);
            }while(counter < MAX_MSG_LEN+2);
            if(counter > MAX_MSG_LEN){
            	printf("Maximum Message Exceed!\n");
            	fflush(stdin);
            }else{
	            printf("+--------------------------------\n");
	            DATA *sendingMsg = newHeader();
	            sendingMsg->command = MSG;
	            addMsg(sendingMsg, newMsg(str, strlen((char *)str)));
	            send_data_buff(socket_id[i], sendingMsg, 0, sockBuff[i]);
	            printf("[ Message (%d bytes) sent to '%s' ]\n", (int)strlen((char *)str), tmpName);
	            freeData(sendingMsg);
            }
        }
}

void chat(){
    int i;
    
    printf("Chat with ? [Enter to return to menu] >> ");
    getchar();
    fgets(tmpName, NAME_LEN+2, stdin);
    // check valid input
    for(i=0;i<NAME_LEN+1;i++){
        if(tmpName[i] >= 'a' && tmpName[i] <='z') continue;
        if(tmpName[i] >= 'A' && tmpName[i] <='Z') continue;
        if(tmpName[i] >= '0' && tmpName[i] <='9') continue;
        if(tmpName[i] =='\n'){ // end of str
            tmpName[i]=0;
            break;
        }
        fflush(stdin);
        tmpName[0]=0;
    }

	if(!strcmp(tmpName, (char *)name)){ // dont be the poison boy
		printf("Don't try to talk with yourself!!!\n");
	}else if(i){
        startConn();
    }
}

void loginedMenu(){
    int choice;
    do{
        do{
            printf("+--- Menu ----------------+\n");
            printf("| 1) List of online users |\n");
            printf("| 2) Read unread message  |\n");
            printf("| 3) Chat with ...        |\n");
            printf("| 4) Go Offline           |\n");
            printf("+-------------------------+\n");
            printf("Your choice >> ");
            if(scanf("%i", &choice)){
                if(choice > 0 && choice < 5) break;
            }else if(getchar()==EOF){
                choice = 4;
                printf("Standard Input Closed.\n");
                break;
            }
            choice=0;
            printf("Please enter valid number!\n");
        }while(!choice);
        switch(choice){
            case 1:
                showClientList();
                break;
            case 2:
                readMsg();
                break;
            case 3:
                chat();
                break;
            case 4:
                closeAllConn();
                printf("[ Bye. ]\n");
        }
    }while(choice != 4);
}

// close all thread and socket
// 1. close all passive and active client
// 2. close listen
// 3. reset socket_id
// 3. close server connection // no need, caller should be server thread (main)
// 4. wait for all thread completed
// 5. reset all thread id
void closeAllConn(){
    static int i;

	pthread_mutex_lock(&conn_mutex);
    printf("[ Logging off ... ");
    close(socket_serv);

    printf("done ]\n[ Disconnecting from all client ... ");
    for(i=MAX_CLIENT; i>=0; i--){
    	if(socket_id[i]){
    		shutdown(socket_id[i],2);
    		//close(socket_id[i]);
	        socket_id[i]=0; // reset socket_id
    	}
    }
    printf("done ]\n");

    for(i=MAX_CLIENT; i>=0; i--){
    	if(connInfo[i]){
			freeData(connInfo[i]);
			connInfo[i]=0;
    	}
    }
    for(i=MAX_CLIENT; i>=0; i--){
    	if(thread[i]){
	        pthread_join(thread[i], NULL); // wait for all thread end by themselves
	        thread[i]=0; // reset thread
    	}
    }

	pthread_mutex_unlock(&conn_mutex);
}


int main(int argc, char** argv){
    // Menu option saver
    int choice;
    int i;
    signal_init();
    if(argc < 3){
        printf("Usage: client.out [IP Address] [Port]\n");
        exit(0);
    }
    
    // init server addr
    memset(&server_addr,0,sizeof(server_addr));
    server_addr.sin_family=AF_INET;
    server_addr.sin_addr.s_addr=inet_addr(argv[1]);
    server_addr.sin_port=htons(atoi(argv[2]));
    
    if(server_addr.sin_port == 0 || server_addr.sin_addr.s_addr ==  ( in_addr_t)-1){
        printf("Invalid [IP Address] or [Port]");
        printf("Usage: client.out [IP Address] [Port]\n");
        exit(0);
    }

    
    while(1){
        port=0; // reset all
        otherMsg = 0;
        for(i=11;i>=0;i--){
            socket_id[i]=0;
            if(i==11) continue;
            sockBuff[i][0] = 0;
            thread[i] = 0;
            connInfo[i] = 0;
        }
        while(1){
            printf("+--- Menu ----------------+\n");
            printf("| 1) Go online            |\n");
            printf("| 2) Quit                 |\n");
            printf("+-------------------------+\n");
            printf("Your choice >> ");
            if(scanf("%i", &choice)){
                if(choice > 0 && choice < 3) break;
            }else if(getchar()==EOF){
                choice = 2;
                printf("Standard Input Closed.\n");
                break;
            }
            printf("Please enter valid number!\n");
        }
        if(choice & 1){
            do{
                getchar();
                printf("Screen name [enter to return to menu] >> ");
                //fgets(name, NAME_LEN+2, stdin);
                fgets(name, sizeof(name), stdin);
                // check valid input
                for(i=0;i<strlen(name);i++){
                    if(name[i] >= 'a' && name[i] <='z') continue;
                    if(name[i] >= 'A' && name[i] <='Z') continue;
                    if(name[i] >= '0' && name[i] <='9') continue;
                    if(name[i] =='\n'){ // end of str
                        name[i]=0;
                        break;
                    }
                    
                    // too long name occur; or
                    // invalid char occur
                    // index :            2 2 2
                    //                    5 5 5
                    //        1 2 3 ..... 5 6 7
                    // input :a b c ..... d a \n   
                    // store :a b c ..... d a 0
                    // too long name
                    fflush(stdin);
                    name[0]=0;
                }
            }while(!name[0]);
            if(!i) continue; // press enter only
            goOnline();
        }else{
            printf("[ Have a nice day. ]\n");
            break;
        }
    }
    // end of program
    return 0;
}
