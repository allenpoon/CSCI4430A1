# include <stdio.h>
# include <stdlib.h>
# include <unistd.h>
# include <string.h>
# include <errno.h>
# include <sys/socket.h>
# include <sys/types.h>
# include <netinet/in.h>

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
		printf("connection error: %s (Errno:%d)\n",strerror(errno),errno);
		exit(0);
	}

	int addr_size = sizeof(client_addr);
	getsockname(sd, (struct sockaddr *) &client_addr, &addr_size);
	printf(" (%d) ... done ]\n", ntohs ( ((struct sockaddr_in *)&client_addr)->sin_port ));

	int choice;
	char name[256];
	int len;
	
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
				if((len=send(sd,name,strlen(name),0))<0){
					printf("Send Error: %s (Errno:%d)\n",strerror(errno),errno);
					exit(0);
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
