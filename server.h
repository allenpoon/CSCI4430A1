// Port number = 12300 + Group Number
#define PORT 12310
#define TRUE 1
#define FALSE 0

typedef int bool;

typedef struct _peer{
	bool online;
	int id;
	int client_sd;
	int sd;
	char *name;
	unsigned short port;
	struct sockaddr_in client_addr;
}PEER;