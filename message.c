ARG *newMsg(char *str, int strLen){
    ARG *arg = malloc(sizeof(ARG));
    arg->name=0;
    strncpy(arg->msg, str, strLen>255?255:strLen);
    arg->msg[255]=0; // prevent overflow
    return arg;
}

ARG *newClient(char *name, unsigned int ip, unsigned short port, int nameLen){
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
    if(header){
        if(header->arg == arg){
            header->arg=arg->arg;
            freeArg(arg);
        }else{
            tmp=header->arg;
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
                *result=LOGIN;
                *((int *)(result+1)) = data->arg->nameLen;
                strncpy(result+1+4, data->arg->name, data->arg->nameLen);
            // Add NULL char to state termination of String
                *(result+1+4+data->arg->nameLen-1) = '\0';
                *((int *)(result+1+4+data->arg->nameLen)) = 2;
                *((short *)(result+1+4+data->arg->nameLen+4)) = data->arg->port;
                *(result+1+4+data->arg->nameLen+4+2) = 0;
                break;
            case LOGIN_OK:
            case GET_LIST:
            case HELLO_OK:
                *result=data->command;
                *(result+1) = 0;
                break;
            case GET_LIST_OK:
                *result=LOGIN;
                counter = ((int *)(result+1));
                *counter = 0;
                arg = data->arg;
                // shift 5 byte
                result = result + 5;
                
                for(i=0;i<10 && arg;i++){
                    *(int *)(result)= arg->nameLen;
                    *counter += 4 + arg->nameLen+4+2; // update arg size
                    strncpy(result+4, arg->name, arg->nameLen);
                    *(int *)(result+4+arg->nameLen) = arg->ip;
                    *(short *)(result+4+arg->nameLen+4) = arg->port;
                    arg=arg->arg;
                    *(result = result+4+arg->nameLen+4+2) = 0; // shift (4+arg->nameLen+4+2) byte and set null to end
                    
                }
                break;
            case HELLO:
                *result=HELLO;
                *((int *)(result+1)) = data->arg->nameLen;
                strncpy(result+1+4, data->arg->name, data->arg->nameLen);
                *(result+1+4+data->arg->nameLen) = 0;
                break;
            case MSG:
                *result=MSG;
                *((int *)(result+1)) = strlen(data->arg->msg);
                strncpy(result+1+4, data->arg->msg,255);
                *(result+1+4+256) = 0;
                break;
            case ERROR:
                result[0]=ERROR;
                *((int *)(result+1))=1;
                result[5]=data->error;
                result[6]=0;
                break;
        }
    }
}


// data[2656]
// return 0 == cannot allocate memory
DATA *parseData(unsigned char *data){
    int counter=0;
    int i=0;
    DATA *result=newHeader();
    result->command = *data;
    switch(*data){
        case LOGIN:

            addClient(result, newClient(
                data+1+4, // name
                0,        // ip
                *((short *)(data+1+4+*((int *)(data+1))+4)), // port
                *((int *)(data+1)) // nameLen
            ));    
            break;
        case LOGIN_OK:
        case GET_LIST:
        case HELLO_OK:
            // completed lol
            break;
        case GET_LIST_OK:
            // shift 5 byte
            counter = *((int *)(data+1));
            data = data + 5;
            
            for(i=0;i<10 && counter > 0;i++){
                addClient(result, newClient(
                                    data+4,
                                    *(int *)(data+4+*(int *)(data)), // ip
                                    *(short *)(data+4+*(int *)(data)+4), // port
                                    *(int *)(data) // nameLen
                        ));
                counter -= 4 + *(int *)(data)+4+2;
            }
            break;
        case HELLO:
            addClient(result, newClient(
                                data+1+4, // name
                                0,        // ip
                                0,        // port
                                *((int *)(data+1)) // nameLen
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
    return header;
}

int freeData(DATA *data){
    if(data->arg){
        freeArg(data->arg);
    }
    free(data);
    return 1;
}

int freeArg(ARG * arg){
    if(arg->arg){
        freeArg(arg->arg);
    }
    if(arg->name){
        free(arg->name);
    }
    free(arg);
    return 1;
}

<<<<<<< HEAD
=======
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
}
>>>>>>> f91cc1a6f4e97b6b612af5ebbc089ce811239c38

char *getClientAddr(struct sockaddr_in * client_addr){
    static char ip[19];
    sprintf(ip, "%d.%d.%d.%d",
        (int)(client_addr->sin_addr.s_addr & 0xff), 
        (int)((client_addr->sin_addr.s_addr & 0xff00)>>8), 
        (int)((client_addr->sin_addr.s_addr & 0xff0000)>>16), 
        (int)((client_addr->sin_addr.s_addr & 0xff000000)>>24));
    return (char *) &ip;
}
<<<<<<< HEAD

#endif
=======
>>>>>>> f91cc1a6f4e97b6b612af5ebbc089ce811239c38
