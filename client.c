# include <stdio.h>
# include <stdlib.h>
# include <unistd.h>
# include <string.h>
# include <errno.h>
# include <sys/socket.h>
# include <sys/types.h>
# include <netinet/in.h>
# include <pthread.h>
# include "message.h"

#define NAME_LEN 255
#define MAX_MSG_LEN 255
#define BUFF_LEN 2656
#define MAX_CLIENT 10

# define IPADDR "127.0.0.1"
# define PORT 12310



// self info
// NAME_LEN + 1 end char + 1 check too long name char
char name[NAME_LEN+2];
char tmpName[NAME_LEN+2];
struct sockaddr_in server_addr;
struct sockaddr_in local_addr;
struct sockaddr_in tmp_addr;


// listening port info
short port=0;
pthread_mutex_t port_mutex = PTHREAD_MUTEX_INITIALIZER;


// 1 server + 10 client
// 0 == server, 1..10 == client
// MAX 9 Conn, 1 client is accept for kick it out
DATA *connInfo[MAX_CLIENT+1]; 

// prevent passive and active client login in the same time
pthread_mutex_t conn_mutex = PTHREAD_MUTEX_INITIALIZER;

// 0 == listening, 1..10 == passive+active
pthread_t thread[MAX_CLIENT+1]; 

// 0 == listen, 1..10 == client
int socket_id[MAX_CLIENT+1];
int socket_serv;
// 0 == server, 1..10 == client
unsigned char sockBuff[MAX_CLIENT+1][BUFF_LEN];

// new active client data passing
char *newClientName;

void clientRecver(void * id){
	int thread_id = *(int *) id;
	
}

// a active client thread
int activeClient(int thread_id){
	ARG *tmp = connInfo[0]->arg;
	tmp = connInfo[0]->arg;
	int i;
	// finding requested client
	while(tmp && strcmp(tmp->name, tmpName)){
		tmp = tmp->arg;
	}
	if(tmp){
		socket_id[thread_id]=socket(AF_INET,SOCK_STREAM,0);

		tmp_addr.sin_family=AF_INET;
		tmp_addr.sin_addr.s_addr=tmp->ip;
		tmp_addr.sin_port=tmp->port;
		
		for(i=0;i<5 && connect(socket_id[thread_id],(struct sockaddr *)&tmp_addr,sizeof(tmp_addr))<0 ;i++){
			printf("connection error: %s (Errno:%d)\n",strerror(errno),errno);
			printf("Retry Connecting to [%s:%hd] ...\n", inet_ntoa(tmp_addr), tmp_addr.sin_port);
		}
		if(i==5){
	        printf("Cannot Connect to [%s:%hd].\n", inet_ntoa(tmp_addr), tmp_addr.sin_port);
	    }else{
			printf("Connecting to '%s' ... ", tmpName);
			connInfo[thread_id]=newHeader();
			connInfo[thread_id]->command = HELLO;
			addClient(connInfo[thread_id], newClient(name, 0, 0, strlen(name)));
			toData(connInfo[thread_id], sockBuff[thread_id]);
			freeData(connInfo[thread_id]);
			
			i=getDataLen(sockBuff[thread_id]);// something error handling
	        i==send(connInfo[thread_id], sockBuff[thread_id], i, 0);
	// something error handling
	        i=recv(connInfo[thread_id], sockBuff[thread_id], BUFF_LEN, 0);
	// something error handling
	//        if(i==getDataLen(sockBuff[0]));
			connInfo[thread_id] = parseData(sockBuff[0]);
			switch(connInfo[thread_id]->command){
	            case HELLO_OK:
	                printf("Done\n");
	                freeData(connInfo[thread_id]);
	                connInfo[thread_id] = newHeader();
					connInfo[thread_id]->command = MSG;
					pthread_create(&thread[thread_id], 0, clientRecver, (void *)&thread_id);
	                break;
	            case ERROR:
				default:
	                if(connInfo[thread_id]->error == NOT_IN_LIST){
	                    printf("Sorry, You are not in Server List.\n");
	                }else{
	                	printf("Sorry, Unknowed error occur.\n");
	                }
	                
	                freeData(connInfo[thread_id]);
	                connInfo[thread_id]=0;
	                
	                close(socket_id[thread_id]);
	                socket_id[thread_id] =0;
	                
	                break;
	        }
	    }
	}
}

// a passive client thread
int passiveClient(){
    
}

int listening(){
    
}


int main(int argc, char** argv){
    // Menu option saver
	int choice;
	int i;
	
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
        port=0; // reset port
        for(i=0;i<12;i++){
            socket_id[i]=0;
            if(i==11) continue;
            sockBuff[i][0]=connInfo[i]=thread[i]=0;
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
                printf("Screen name [enter to return to menu] >> ");
        		fgets(name, NAME_LEN+2, stdin);
        		// check valid input
        		for(i=0;i<NAME_LEN+1;i++){
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
            printf("Bye.\n");
            break;
        }
	}
    // end of program
    return 0;
}

void goOnline(){
    DATA *tmpData;
	int i;
    // start listen
    pthread_create(&thread[0], NULL, listening, NULL);
    
    // waiting for listen thread after binding port
    while(!port){
        // waiting if listen thread locked
        pthread_mutex_lock(port_mutex); 
        // release lock, prevent dead lock
        // or give time for listen thread lock
        pthread_mutex_unlock(port_mutex);
        sleep(0);
    }
    
    // connect to server
    socket_serv = socket(AF_INET,SOCK_STREAM,0);
	for(i=0;i<5 && connect(socket_serv,(struct sockaddr *)&server_addr,sizeof(server_addr))<0 ;i++){
		printf("connection error: %s (Errno:%d)\n",strerror(errno),errno);
		printf("Retry Connecting to [%s:%hd] ...\n", inet_ntoa(server_addr), server_addr.sin_port);
	}
	if(i==5){
        printf("Cannot Connect to [%s:%hd].\n", inet_ntoa(server_addr), server_addr.sin_port);
    }else{
        // connection completed
        // ask for user action
        printf("Logging in ... ");
        
        i=sizeof(local_addr);
        getsockname(socket_serv, (struct sockaddr *) &local_addr, &i);
        
        tmpData = newHeader();
        tmpData->command = LOGIN;
        addClient(tmpData, newClient(name, 0, local_addr.sin_port, strlen(tmpData));
        toData(tmpData, sockBuff[0]);
        freeData(tmpData);
        
        i=getDataLen(sockBuff[0]);
// something error handling
        i==send(socket_serv, sockBuff[0], i, 0);
// something error handling
        i=recv(socket_serv, sockBuff[0], BUFF_LEN, 0);
// something error handling
//        if(i==getDataLen(sockBuff[0]));
        tmpData = parseData(sockBuff[0]);
        switch(tmpData->command){
            case LOGIN_OK:
                printf("\"Hello, %s!\"\n", name);
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
// close all socket, close all thread, free memory
}

void loginedMenu(){
    int choice;
    do{
        do{
    		printf("+--- Menu ----------------+\n");
    		printf("| 1) List of online users |\n");
    		printf("| 2) Read unread message  |\n");
    		printf("| 3) Chat with ...        |\n");
    		printf("| 4) Quit                 |\n");
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
        }
    }while(choice != 4);
}

void showClientList(){
    DATA *tmpData;
    ARG * tmpArg;
    int i;
    printf("Retrieving from server .... ");
    tmpData = newHeader();
    tmpData->command = GET_LIST;
    toData(tmpData, sockBuff[0]);
    freeData(tmpData);
    i=getDataLen(sockBuff[0]);
// something error handling
    i==send(socket_serv, sockBuff[0], i, 0); 
    i=recv(socket_serv, sockBuff[0], BUFF_LEN, 0);
// something error handling
//    if(i==getDataLen(sockBuff[0]));
    tmpData = parseData(sockBuff[0]);
    printf("Done.");
    if(tmpData->command == GET_LIST_OK){
        if(connInfo[0]){
            freeData(connInfo[0]);
        }
        connInfo[0]=tmpData;
        tmpArg = tmpData->arg;
        i=1;
		printf("\n");
		printf("+--- Online List ----------------\n");
		do{
		    printf("| %2d) %255s\n", i++, tmpArg->name);
		    tmpArg=tmpArg->arg;
        }while(tmpArg);
		printf("+--------------------------------\n");
    }else{
        printf("Get List Error occur, unknown command id.");
    }
}

void readMsg(){
	int i,j;
	printf("\n");
	for(i=1;i<=MAX_CLIENT;i++){
		if(connInfo[i] && connInfo[i]->arg){
			j=0;
			printf("+--- Message From '%s'\n", connInfo[i]->name);
			while(connInfo[i]->arg){
				printf("+------- #%d\n", ++j);
				printf("%s", connInfo[i]->arg->msg);
// need to set mutex
				removeArg(connInfo[i], connInfo[i]->arg);
			}
			printf("+--------------------------------\n\n", connInfo[i]->name);
		}
	}
	printf("End of all unreaded messages.\n\n");
}

void chat(){
	int i;
	
    do{
        printf("Chat with ? [Enter to return to menu] >> ");
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
    }while(!tmpName[0]);
    if(i){
		startConn();
	}
}

void startConn(){
	int i,counter;
	char *str = sockBuff[0];

// connection here
	
	for(i=1;i<MAX_CLIENT + 1 && !(connInfo[i] && !strcmp(connInfo[i]->arg->name, tmpName));i++);
	if(i>MAX_CLIENT){
    	pthread_mutex_lock(conn_mutex);
		for(i=1;j && i<MAX_CLIENT + 1;i++){
			if(!thread[i]){
// create active client
				activeClient(i);
				break;
			}
		}
    	pthread_mutex_unlock(conn_mutex);
	}
		
		// thread exist if [ 0 < i < MAX_CLIENT+1 ]
		if(i<MAX_CLIENT+1){
			counter =0;
			printf("\n");
			printf("+---Type your message to '%s'-------------------\n", tmpName);
			do{
				fgets(str+counter, MAX_MSG_LEN+1, stdin);
				if(!strcmp(".\n", str+counter)){
					*(str+counter) = 0;
					break;
				}
				counter += strlen(str+counter);
			}while(counter < MAX_MSG_LEN);
			j=0;
		}
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
    for(i=MAX_CLIENT; i>=0; i--){
        close(socket_id[i]);
        socket_id[i]=0; // reset socket_id
    }
    for(i=MAX_CLIENT; i>=0; i--){
        thread[i] && pthread_join(thread[i]); // wait for all thread end by themselves
        thread[i]=0; // reset thread
    }
}
