ARG *newMsg(char *str, unsigned int strLen){
    ARG *arg = malloc(sizeof(ARG));
    arg->name=0;
    strncpy(arg->msg, str, strLen>255?255:strLen);
    arg->msg[255]=0; // prevent overflow
    return arg;
}

ARG *newClient(char *name, unsigned int ip, unsigned short port, unsigned int nameLen){
    ARG *arg = malloc(sizeof(ARG));
    arg->nameLen=nameLen;
    arg->name=malloc(sizeof(char)*nameLen);
    strncpy(arg->name, name, nameLen);
    arg->ip=ip;
    arg->port=port;
    return arg;
}


// return 1 == success
// return 0 == msg too long
// return -1 == worng arg
// return -2 == null pointer exception
short addMsg(DATA *header, ARG *msg){
    int i=1;
    ARG *temp;
    if(header && msg){
        if(!msg->name && header->command & MSG){
            if(header->arg){
                freeArg(msg);
                return 0;
            }
            header->arg=msg;
            return 1;
        }
        return -1;
    }
    return -2;
}

// return 1 == success
// return 0 == list full
// return -1 == worng type (arg)
// return -2 == null pointer exception
short addClient(DATA *header, ARG *client){
    int i=1;
    ARG *temp;
    if(header && client){
        if(client->name && (header->command == GET_LIST_OK || header->command == LOGIN)){
            if(header->arg){
                temp = header->arg;
                for(;temp->arg;i++){
                    temp=temp->arg;
                }
                i<10 && (temp->arg=client) || freeArg(client);
                return i<10;
            }
            header->arg=client;
            return 1;
        }
        return -1;
    }
    return -2;
}

void removeArg(DATA *data, ARG *arg){
    ARG *tmp;
    if(data){
        if(data->arg == arg){
            data->arg=arg->arg;
            freeArg(arg);
        }else{
            tmp=data->arg;
            while(tmp->arg && tmp->arg != arg) tmp=tmp->arg;
            if(tmp->arg == arg){
                tmp->arg=arg->arg;
                freeArg(arg);
            }
        }
    }
}

// result[2656]
// result = 1 byte COMMAND + 4 byte ARG_SIZE 
//          + 10 * ( 4 byte NAME_LEN + 255 byte NAME + 4 byte IP + 2 byte PORT )
//          + 1 byte NULL
// return 0 == error occur
void toData(DATA *data, unsigned char *result){
    int *counter=0;
    int i=0;
    ARG *arg=0;
    *result=0;
    if(data){
        switch(data->command){
            case LOGIN:
                *                     result                            =LOGIN;
                *((unsigned int *)(   result+1                          )) = data->arg->nameLen;
                strncpy(              result+1+4                        , data->arg->name, data->arg->nameLen);
                *((unsigned int *)(   result+1+4+data->arg->nameLen     )) = 2;
                *((unsigned short *)( result+1+4+data->arg->nameLen+4   )) = data->arg->port;
                *(                    result+1+4+data->arg->nameLen+4+2 ) = 0;
                break;
            case LOGIN_OK:
            case GET_LIST:
            case HELLO_OK:
                * result    =data->command;
                *(result+1  ) = 0;
                break;
            case GET_LIST_OK:
                *                   result =GET_LIST_OK;
                counter = ((int *)( result+1           ));
                
                *counter = 0;
                arg = data->arg;
                
                // shift 5 byte
                result = result + 5;
                for(i=0;i<10 && arg;i++){
                    *counter += 4 + arg->nameLen+4+2; // update arg size
                    
                    *(unsigned int *)(   result                  )= arg->nameLen;
                    strncpy(             result+4                , arg->name, arg->nameLen);
                    *(unsigned int *)(   result+4+arg->nameLen   ) = arg->ip;
                    *(unsigned short *)( result+4+arg->nameLen+4 ) = arg->port;
                    
                    *(result = result+4+arg->nameLen+4+2) = 0; // shift (4+arg->nameLen+4+2) byte and set null to end
                    
                    arg=arg->arg;// next arg
                }
                break;
            case HELLO:
                *                   result                        =HELLO;
                *((unsigned int *)( result+1                      )) = data->arg->nameLen;
                strncpy(            result+1+4                    , data->arg->name, data->arg->nameLen);
                *(                  result+1+4+data->arg->nameLen ) = 0;
                break;
            case MSG:
                *                   result         =MSG;
                *((unsigned int *)( result+1       )) = strlen(data->arg->msg);
                strncpy(            result+1+4     , data->arg->msg,255);
                *(                  result+1+4+256 ) = 0;
                break;
            case ERROR:
                *                   result       =ERROR;
                *((unsigned int *)( result+1     ))=1;
                *(                  result+1+4   )=data->error;
                *(                  result+1+4+1 )=0;
                break;
        }
    }
}


// data[2656]
// return 0 == cannot allocate memory
DATA *parseData(unsigned char *data){
    unsigned int counter=0;
    int i=0;
    DATA *result=newHeader();
    result->command = *data;
    switch(*data){
        case LOGIN:

            addClient(result, newClient(
                                      data+1+4, // name
                                      0,        // ip
                *((unsigned short *)( data+1+4+*((int *)(data+1))+4 )), // port
                *((unsigned int *)(   data+1 )) // nameLen
            ));    
            break;
        case LOGIN_OK:
        case GET_LIST:
        case HELLO_OK:
            // completed lol
            break;
        case GET_LIST_OK:
            // shift 5 byte
            counter = *((unsigned int *)(data+1));
            data = data + 5;
            
            for(i=0;i<10 && counter > 0;i++){
                addClient(result, newClient(
                                                         data+4,
                                    *(unsigned int *)(   data+4+*(unsigned int *)(data)), // ip
                                    *(unsigned short *)( data+4+*(unsigned int *)(data)+4), // port
                                    *(unsigned int *)(   data) // nameLen
                        ));
                counter -= 4 + *(int *)(data)+4+2;
                data +=    4 + *(int *)(data)+4+2;
            }
            break;
        case HELLO:
            addClient(result, newClient(
                                data+1+4, // name
                                0,        // ip
                                0,        // port
                                *((unsigned int *)(data+1)) // nameLen
                     )); 
            break;
        case MSG:
            addMsg(result, newMsg(data+1+4, *(int *)(data+1) )); 
            break;
        case ERROR:
            result->error = *(data+5);
            break;
    }
    return result;
}


DATA * newHeader(){
    DATA *header=malloc(sizeof(DATA));
    header->command=0;
    header->arg=0;
    header->length =0;
    return header;
}

int freeData(DATA *data){
    if(!data){
        if(data->arg){
            freeArg(data->arg);
        }
        free(data);
    }
    return 1;
}

int freeArg(ARG * arg){
    if(!arg){
        if(arg->arg){
            freeArg(arg->arg);
        }
        if(arg->name){
            free(arg->name);
        }
        free(arg);
        arg = NULL;
    }
    return 1;
}

int getDataLen(unsigned char *data){
    int result = 1;
    switch(*data){
        case LOGIN:
            result += 4 + 2;
        case GET_LIST_OK:
        case HELLO:
        case MSG:
        case ERROR:
            result += 4 + *(int *)(data+1);
            break;
        case LOGIN_OK:
        case GET_LIST:
        case HELLO_OK:
            // completed lol
            break;
    }
    return result;
}

char *getClientAddr(struct sockaddr_in * client_addr){
    static char ip[19];
    sprintf(ip, "%d.%d.%d.%d",
        (int)(client_addr->sin_addr.s_addr & 0xff), 
        (int)((client_addr->sin_addr.s_addr & 0xff00)>>8), 
        (int)((client_addr->sin_addr.s_addr & 0xff0000)>>16), 
        (int)((client_addr->sin_addr.s_addr & 0xff000000)>>24));
    return (char *) &ip;
}

int send_data_buff(int sd, DATA * data, unsigned int *rtnlen, unsigned char *buff){
    toData(data, buff);
    if((*rtnlen=send(sd,buff,getDataLen(buff),0))<0){
        printf("Send Error: %s (Errno:%d)\n",strerror(errno),errno);
        return -1;
    }
    return 1;
}
int send_data(int sd, DATA * data, unsigned int *rtnlen){
	// don't try to use static since there are more than one thread may call
    unsigned char * buff = malloc(2656);
    int result = send_data_buff(int sd, DATA * data, int *rtnlen, buff);
    free(buff);
    return result;
}

DATA *recv_data_buff(int sd, unsigned int *rtnlen, int *status, unsigned char *buff){
    if(((*rtnlen)=recv(sd,buff,2656,0))<=0){
        if(errno == 0){
            *status = -2;
            return NULL;
        }
        printf("receive error: %s (Errno:%d)\n", strerror(errno),errno);
        *status = -1;
        return NULL;
    }
    *status = 1;
    return parseData(buff);
}
DATA *recv_data(int sd, unsigned int *rtnlen, int *status){
	// don't try to use static since there are more than one thread may call
    unsigned char * buff = malloc(2656);
    DATA *tmp = recv_data_buff(int sd, int *rtnlen, int *status, unsigned char *buff)
    free(buff);
    return tmp;
}
