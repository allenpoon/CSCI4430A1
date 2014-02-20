#ifndef message_h
#define message_h

#include <string.h>

// server-client COMMAND
#define LOGIN         0x01
#define LOGIN_OK      0x02
#define GET_LIST      0x03
#define GET_LIST_OK   0x04

// client-client COMMAND
#define HELLO         0x10
#define HELLO_OK      0x20
#define MSG           0x30

// common COMMAND
#define ERROR         0xff
#define SAME_NAME     0x01
#define SAME_CONN     0x02
#define TOO_MUCH_CONN 0x03

typedef struct arg{
    int nameLen;
    int ip;
    char *name;    // name != null --> server client list
    short port;    // port 0 for client-to-client connection
    char msg[256]; // name == null --> message
    struct arg *arg;
} ARG;

typedef struct header{
    unsigned char command;
    char error;
    int length;
    ARG *arg;
} DATA;

ARG *newMsg(char *str);
ARG *newClient(char *name, int ip, short port);

// return 1 == success
// return 0 == msg too long
// return -1 == worng arg
// return -2 == null pointer exception
short addMsg(DATA *header, ARG *msg);

// return 1 == success
// return 0 == list full
// return -1 == worng type (arg)
// return -2 == null pointer exception
short addClient(DATA *header, ARG *client);

void getData(DATA *data, char *result);

DATA * newHeader();

int freeData(DATA *data);

int freeArg(ARG * arg);

#include "message.c"
#endif
