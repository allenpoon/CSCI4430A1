#ifndef message_h
#define message_h

typedef struct arg{
    int nameLen;
    char *name;    // name != null --> server client list
    int ip;
    short port;    // port 0 for client-to-client connection
    char msg[256]; // name == null --> message
    struct arg *arg;
} ARG;

typedef struct header{
    char command;
    int length;
    ARG *arg;
} DATA;

ARG *newMsg(char *str);
ARG *newClient(char *name, int ip, short port);
DATA *newData(char command);

DATA *toArg(char **client, int len); // client == list of clients name, len == array length of client != num of client
DATA *toMsg(char *str);

char *getData(DATA *data);

#include "message.c"
#endif
