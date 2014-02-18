#ifdef message_h

ARG *newMsg(char *str){
    ARG *arg = malloc(sizeof(ARG));
    arg->name=0;
    strncpy(arg->msg, str, 255);
    arg->msg[255]=0;
    return arg;
}

ARG *newClient(char *name, int ip, short port){
    ARG *arg = malloc(sizeof(ARG));
    arg->nameLen=strlen(name);
    arg->name=malloc(sizeof(char)*arg->nameLen);
    strncpy(arg->name, name, arg->nameLen);
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
                temp->arg=client;
                return i<10;
            }
            header->arg=client;
            return 1;
        }
        return -1;
    }
    return -2;
}

// result[2656]
// result = 1 byte COMMAND + 4 byte ARG_SIZE 
//          + 10 * ( 4 byte NAME_LEN + 255 byte NAME + 4 byte IP + 2 byte PORT )
//          + 1 byte NULL
// return 0 == error occur
void getData(DATA *data, char *result){
    *result=0;
    if(data){
        switch(data->command){
            case LOGIN:
                *result=LOGIN;
                *((int *)(result+1)) = data->arg->nameLen;
                strncpy(result+1+4, data->arg->name,data->arg->nameLen);
                *((int *)(result+1+4+data->arg->nameLen)) = 2;
                *((short *)(result+1+4+data->arg->nameLen+4)) = data->arg->port;
                result[1+4+data->arg->nameLen+4+2] = 0;
                break;
            case LOGIN_OK:
            case GET_LIST:
            case HELLO_OK:
                *result=data->command;
                *(result+1) = 0;
                break;
            case GET_LIST_OK:
                break;
            case HELLO:
                break;
            case MSG:
                result[0] = MSG;
                *((int *)(result+1)) = strlen(data->arg->msg);
                strncpy(result+5, data->arg->msg,256);
                break;
            case ERROR:
                result[0]=ERROR;
                *((int *)(result+1))=1;
                result[5]=data->error;
                result[6]=0;
                break;
        }
    }
    return result;
}

DATA * newHeader(){
    DATA *header=malloc(sizeof(DATA));
    header->command=0;
    header->arg=0;
    return header;
}

void freeData(DATA *data){
    if(data->arg){
        freeArg(data->arg);
    }
    free(data);
}

void freeArg(ARG * arg){
    if(arg->arg){
        freeArg(arg->arg);
    }
    if(arg-->name){
        free(arg->name);
    }
    free(arg);
}

#endif
