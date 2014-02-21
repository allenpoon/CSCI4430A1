# include <stdio.h>
# include <stdlib.h>
# include <unistd.h>
# include <string.h>
# include <errno.h>
# include <sys/socket.h>
# include <sys/types.h>
# include <netinet/in.h>
# include "message.h"

# define IPADDR "127.0.0.1"
# define PORT 12310

int main(int argc, char** argv){
	/*if(connect(sd,(struct sockaddr *)&server_addr,sizeof(server_addr))<0){
		printf("connection error: %s (Errno:%d)\n",strerror(errno),errno);
		exit(0);
	}*/

	if(argc < 3){
		printf("Usage: client.out [IP Address] [Port]\n");
		exit(0);
	}

	int sd=socket(AF_INET,SOCK_STREAM,0);
	struct sockaddr_in server_addr;
	struct sockaddr_in client_addr;
	memset(&server_addr,0,sizeof(server_addr));
	server_addr.sin_family=AF_INET;
	server_addr.sin_addr.s_addr=inet_addr(argv[1]);
	server_addr.sin_port=htons(atoi(argv[2]));
	printf("[ Opening an arbitrary listening port");
	if(connect(sd,(struct sockaddr *)&server_addr,sizeof(server_addr))<0){
		printf(" ... \nconnection error: %s (Errno:%d)\n",strerror(errno),errno);
		exit(0);
	}

	int addr_size = sizeof(client_addr);
	getsockname(sd, (struct sockaddr *) &client_addr, &addr_size);
	printf(" (%d) ... done ]\n", ntohs ( ((struct sockaddr_in *)&client_addr)->sin_port ));

	int choice;
	char name[256];
	int len, nameLen;
	unsigned char *buff = malloc(2656);
	
	while(1){
		/*Prompt out this stuff	
			+--- Menu ----------------+
			| 1) Go online            |
			| 2) Quit                 |
			+-------------------------+
			Your choice >> 
		*/
		printf("+--- Menu ----------------+\n| 1) Go online            |\n| 2) Quit                 |\n+-------------------------+\nYour choice >> ");
		scanf("%i", &choice);
		while(!(choice > 0 && choice < 3)){
			printf("Please enter valid number!\n+--- Menu ----------------+\n| 1) Go online            |\n| 2) Quit                 |\n+-------------------------+\nYour choice >> ");
			getchar();
			scanf("%i", &choice);
		}

		switch(choice){
			case 2:
				// Enter "2" to exit
				close(sd);
				exit(0);
				break;
			case 1:
				printf("Screen name [enter to return to menu] >> ");
				getchar();
				fgets(name, sizeof(name), stdin);
				if(name[0] =='\n'){
					continue;
				}
				DATA *data = newHeader();
				data->command = LOGIN;
				nameLen = strlen(name);
				data->arg = newClient(name, 0, (unsigned short) ntohs(client_addr.sin_port), nameLen) ;
				data->length = 1+4+4+nameLen+4+2+1;
				toData(data, buff);
				if((len=send(sd,buff,data->length,0))<0){
					printf("Send Error: %s (Errno:%d)\n",strerror(errno),errno);
					continue;
				}

				if((len=recv(sd,buff,2656,0))<=0){
					printf("[CLIENT]receive error: %s (Errno:%d)\n", strerror(errno),errno);
				}

				freeData(data);
				data = parseData(buff);
				*(name+nameLen-1) = '\0';

				switch(data->command){
					case LOGIN_OK:
						printf("LOGIN OK!\n");
						printf("[ Logging in .... \"Hello, %s!\" ]\n", name);
						break;
					case ERROR:
						switch(data->error){
							case SAME_NAME:
								printf("[ Logging in .... \"Sorry, \'%s\' is already there!\" ]\n", name);
								break;
							case SAME_CONN:
								printf("[ Logging in .... \"Sorry, a connection regarding \'%s:%d\' is already there!\" ]\n", 
									getClientAddr(&client_addr),
									ntohs ( ((struct sockaddr_in *)&client_addr)->sin_port ));
								break;
							case TOO_MUCH_CONN:
								printf("[ Logging in .... \"Sorry, maximum number of online clients is reached!\" ]\n", name);
								break;
						}
						break;
				}

				do{
					if(data->command == LOGIN_OK){
						printf("+--- Menu ----------------+\n| 1) List of online users |\n| 2) Read unread message  |\n| 3) Chat with ...        |\n| 4) Quit                 |\n+-------------------------+\nYour choice >> ");
						scanf("%d", &choice);
					}
				}while(choice <= 0 || choice > 4);

				switch(choice){
					case 1:
						break;
					case 2:
						break;
					case 3:
						break;
					case 4:
						close(sd);
						exit(0);
						break;
				}

				close(0);
				exit(0);
				break;
		}
	}

	while(1){
		char buff[100];
		memset(buff,0,100);
		scanf("%s",buff);
		//printf("%s\n",buff);
		int len;
		if((len=send(sd,buff,strlen(buff),0))<0){
			printf("Send Error: %s (Errno:%d)\n",strerror(errno),errno);
			exit(0);
		}
	}
	return 0;
}
