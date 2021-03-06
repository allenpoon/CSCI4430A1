ARG *newMsg(unsigned char *str, unsigned int strLen){
    ARG *arg = malloc(sizeof(ARG));
    arg->name=0;
    strncpy((char *)arg->msg, (char *)str, strLen>255?255:strLen);
    arg->msg[255]=0; // prevent overflow
    return arg;
}

ARG *newClient(unsigned char *name, unsigned long ip, unsigned short port, unsigned int nameLen){
    ARG *arg = malloc(sizeof(ARG));
    arg->nameLen=nameLen;
    arg->name=malloc(sizeof(char)*nameLen);
    strncpy((char *)arg->name, (char *)name, nameLen);
    *(arg->name+nameLen) = 0;
    arg->ip=ip;
    arg->port=port;
    return arg;
}


// return 1 == success
// return 0 == msg too long
// return -1 == worng arg
// return -2 == null pointer exception
short addMsg(DATA *header, ARG *msg){
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
// return 0 == msg too long
// return -1 == worng arg
// return -2 == null pointer exception
short addArg(DATA *header, ARG *arg){
    ARG *temp;
    if(header && arg){
        if(header->arg){
            temp=header->arg;
            for(;temp->arg;){
                temp=temp->arg;
            }
            temp->arg=arg;
        }else{
        	header->arg=arg;
        }
        return 1;
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
        if(client->name && (header->command == GET_LIST_OK || header->command == LOGIN || header->command == HELLO )){
            if(header->arg){
                temp = header->arg;
                for(;temp->arg;i++){
                    temp=temp->arg;
                }
                if(i<10) temp->arg=client; else freeArg(client);
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
            arg->arg = 0;
            freeArg(arg);
        }else{
            tmp=data->arg;
            while(tmp->arg && tmp->arg != arg) tmp=tmp->arg;
            if(tmp->arg == arg){
                tmp->arg=arg->arg;
            	arg->arg = 0;
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
            	//printf("a\n");
                *((   result+1                          )) = (unsigned char)(data->arg->nameLen) & 0xff;
                *((   result+1+1                        )) = (unsigned char)(data->arg->nameLen) >> 8 & 0xff;
                *((   result+1+2                        )) = (unsigned char)(data->arg->nameLen) >> 16 & 0xff;
                *((   result+1+3                        )) = (unsigned char)(data->arg->nameLen) >> 24 & 0xff;
            	//printf("a\n");
                strncpy(   (char *)   result+1+4                        , (char *)data->arg->name, data->arg->nameLen);
            	//printf("a\n");
                //*((unsigned int *)(   result+1+4+data->arg->nameLen                          )) = htonl(2);
                *((   result+1+4+data->arg->nameLen                           )) = (unsigned char)(2) & 0xff;
                *((   result+1+4+data->arg->nameLen +1                        )) = (unsigned char)(2) >> 8 & 0xff;
                *((   result+1+4+data->arg->nameLen +2                        )) = (unsigned char)(2) >> 16 & 0xff;
                *((   result+1+4+data->arg->nameLen +3                        )) = (unsigned char)(2) >> 24 & 0xff;

            	//printf("a\n");
                //*((unsigned short *)( result+1+4+data->arg->nameLen+4                          )) = (data->arg->port) ;
                *((   result+1+4+data->arg->nameLen+4                           )) = (unsigned char)(data->arg->port) & 0xff;
                *((   result+1+4+data->arg->nameLen+4 +1                        )) = (unsigned char)(data->arg->port) >> 8 & 0xff;
            	//printf("a\n");
                *(                    result+1+4+data->arg->nameLen+4+2 ) = 0;
            	//printf("a\n");
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
                    
                    //*(unsigned int *)(   result                  )= (arg->nameLen);
                    *((   result                            )) = (unsigned char)(arg->nameLen) & 0xff;
                	*((   result  +1                        )) = (unsigned char)(arg->nameLen) >> 8 & 0xff;
                	*((   result  +2                        )) = (unsigned char)(arg->nameLen) >> 16 & 0xff;
                	*((   result  +3                        )) = (unsigned char)(arg->nameLen) >> 24 & 0xff;
                    strncpy(  (char *)   result+4                , (char *)arg->name, arg->nameLen);
                    //*(unsigned long *)(  result+4+arg->nameLen   ) = (arg->ip);
                    *((   result+4+arg->nameLen                           )) = (unsigned char)(arg->nameLen) & 0xff;
                	*((   result+4+arg->nameLen +1                        )) = (unsigned char)(arg->nameLen) >> 8 & 0xff;
                	*((   result+4+arg->nameLen +2                        )) = (unsigned char)(arg->nameLen) >> 16 & 0xff;
                	*((   result+4+arg->nameLen +3                        )) = (unsigned char)(arg->nameLen) >> 24 & 0xff;
                    *(unsigned short *)( result+4+arg->nameLen+4 ) = (arg->port);
                    *((   result+4+arg->nameLen+4                           )) = (unsigned char)(arg->port) & 0xff;
                	*((   result+4+arg->nameLen+4 +1                        )) = (unsigned char)(arg->port) >> 8 & 0xff;
                    
                    *(result = result+4+arg->nameLen+4+2) = 0; // shift (4+arg->nameLen+4+2) byte and set null to end
                    
                    arg=arg->arg;// next arg
                }
                break;
            case HELLO:
                *                   result                        =HELLO;
                //*((unsigned int *)( result+1                      )) = (data->arg->nameLen);
                *((   result+1                           )) = (unsigned char)(data->arg->nameLen) & 0xff;
            	*((   result+1 +1                        )) = (unsigned char)(data->arg->nameLen) >> 8 & 0xff;
            	*((   result+1 +2                        )) = (unsigned char)(data->arg->nameLen) >> 16 & 0xff;
            	*((   result+1 +3                        )) = (unsigned char)(data->arg->nameLen) >> 24 & 0xff;
                strncpy( (char *)   result+1+4                    , (char *)data->arg->name, data->arg->nameLen);
                *(                  result+1+4+data->arg->nameLen ) = 0;
                break;
            case MSG:
                *                   result         =MSG;
                //*((unsigned int *)( result+1       )) = (strlen((char *)data->arg->msg));
                *((   result+1                           )) = (unsigned char)(strlen((char *)data->arg->msg)) & 0xff;
            	*((   result+1 +1                        )) = (unsigned char)(strlen((char *)data->arg->msg)) >> 8 & 0xff;
            	*((   result+1 +2                        )) = (unsigned char)(strlen((char *)data->arg->msg)) >> 16 & 0xff;
            	*((   result+1 +3                        )) = (unsigned char)(strlen((char *)data->arg->msg)) >> 24 & 0xff;
                strncpy(  (char *)   result+1+4     , (char *)data->arg->msg,255);
                *(                  result+1+4+256 ) = 0;
                break;
            case ERROR:
                *                   result       =ERROR;
                //*((unsigned int *)( result+1     ))=1;
                *((   result+1                           )) = (unsigned char)(1) & 0xff;
            	*((   result+1 +1                        )) = (unsigned char)(1) >> 8 & 0xff;
            	*((   result+1 +2                        )) = (unsigned char)(1) >> 16 & 0xff;
            	*((   result+1 +3                        )) = (unsigned char)(1) >> 24 & 0xff;
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
                                    (*(unsigned long *)(  data+4+*(unsigned int *)(data))), // ip
                                    (*(unsigned short *)( data+4+*(unsigned int *)(data)+4)), // port
                                    (*(unsigned int *)(   data)) // nameLen
                        ));
                counter -= 4 + (*(unsigned int *)(data))+4+2;
                data +=    4 + (*(unsigned int *)(data))+4+2;
            }
            break;
        case HELLO:
            addClient(result, newClient(
                                data+1+4, // name
                                0,        // ip
                                0,        // port
                                (*((unsigned int *)(data+1))) // nameLen
                     )); 
            break;
        case MSG:
            addMsg(result, newMsg(data+1+4, (*(unsigned int *)(data+1)) ));
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
    header->next =0;
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
    sprintf(ip,"%s",inet_ntoa(client_addr->sin_addr));
    return (char *) &ip;
}

int send_data_buff(int sd, DATA * data, unsigned int *rtnlen, unsigned char *buff){
	static unsigned int i;
	printf("hi\n");
    toData(data, buff);
	printf("hi sending thru %d\n", sd);
    if(!rtnlen) rtnlen=malloc(sizeof(unsigned int));
	printf("hi\n");
    printf("SD: %d, BuffLen: %d\n", sd, getDataLen(buff));
    if(((*rtnlen)=send(sd,buff,getDataLen(buff),0))<0){
        printf("Send Error: %s (Errno:%d)\n",strerror(errno),errno);
        return 0;
    }
    printf("SENDING\n");
    return 1;
}
int send_data(int sd, DATA * data, unsigned int *rtnlen){
	// don't try to use static since there are more than one thread may call
    unsigned char * buff = malloc(2656);
    int result = send_data_buff(sd, data, rtnlen, buff);
    free(buff);
    return result;
}

DATA *recv_data_buff(int sd, unsigned int *rtnlen, int *status, unsigned char *buff){
	static unsigned int i;
    if(!rtnlen) rtnlen=&i;
    if(!status) status=(int *)&i;
    if((*rtnlen=recv(sd,buff,2656,0))<=0){
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
    DATA *tmp = recv_data_buff(sd, rtnlen, status, buff);
    free(buff);
    return tmp;
}
