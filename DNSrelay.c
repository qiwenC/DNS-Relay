#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h> /* socket(),sendto(),recvfrom()*/
#include <arpa/inet.h> /* sockaddr_in and inet_addr() */

#define bMAX 512 /* longest string to DNS request */
#define qMAX 300
typedef struct _DATABASE{
    char string[100];
}_DATABASE;

_DATABASE data[qMAX];

typedef struct _DNS_QER{ //DNS query
    char domName[68]; /*query domain name*/ //国际标准最长域名为68字节
    char type[2]; /*type of the query */ // A 为 IPv4， AAAA 为 IPv6
    char classes[2]; /*classes of the query*/ // IN Internet system
}_DNS_QER;

typedef struct _DNS_RR{
    char domName[68]; /*query domain name*/ //国际标准最长域名为68字节
    unsigned short type; /*type of the query */ // A 为 IPv4， AAAA 为 IPv6
    unsigned short classes; /*classes of the query*/ // IN Internet system
    unsigned int ttl;/*time to live*/
    unsigned int ipAddr;/*IPv4 address*/
    unsigned short dataLen;/*resource data length*/
}_DNS_RR;

typedef struct _ID{
    unsigned short oldID;
    unsigned short newID;
}_ID;



int loadFile();
int checkType(char *);
void getQuery(char *, struct _DNS_QER *);
int retIP(char *,char *,int);
int ipVaild(char *);
void initResp(struct _DNS_RR *,struct _DNS_QER,char *);
int consResp(struct _DNS_RR ,char *,char *,int);

int main(int argc, const char * argv[]) {
    int sock; /*socket*/
    int flag_ip = -1; /*a flag that verify the type of the DNS 1 for IPv4, 0 for others*/
    int flag_existip = -1; /*a flag that verify the domain name is in database or not. 1 for in databse*/
    int op = -1;/*a flag that verify the option*/
    struct sockaddr_in srvAddr; /*local server address*/
    struct sockaddr_in cliAddr; /*client address*/
    struct sockaddr_in dnsAddr; /*local dns address*/
    struct sockaddr_in respAddr; /*response ip address*/
    char *localDNS = "202.106.0.20";
    char *localAddr = "10.0.2.15";
    char ipAddr[15]="\0";/*the ip address corresponding to the domain name*/
    unsigned int cliAddrLen; /*length of client address*/
    unsigned int dnsAddrLen; /*length of dns address*/
    int recvMsgSize; /* size of received message */
    unsigned short port; /*sever port*/
    char buffer[bMAX]; /* Buffer for string */
    port = 53; /*port number*/
    char response[bMAX];/*Buffer for response*/
    int rspMsgSize; /*size of the response message*/
    struct _DNS_QER query; /*DNS query*/
    struct _DNS_RR answer; /*DNS anwser*/
    int dataSize = 0; /*size of the database*/
    _ID idTable;
    
    
    printf("Port number = %d\n",port);
    
    /*load the local DNS database*/
    dataSize = loadFile();
    /* test
     int l = 0;
    for(l =0;l<dataSize;l++){
        printf("%d!: %s\n",l,data[l].string);
    } end of test*/
    
    /*create socket*/
    sock = socket(PF_INET,SOCK_DGRAM,0); //建立socket应用PF，绑定地址用AF DGRAM无保障面向UDP，STREAM有保障面向TCP
    if(sock < 0){
        printf("socket() failed!");
        exit(1);
    }
    /*construct local address structure*/
    memset(&srvAddr, 0, sizeof(srvAddr)); //初始化操作，清零
    srvAddr.sin_family = AF_INET;
    srvAddr.sin_addr.s_addr = htonl(INADDR_ANY);//监听所有网卡port端口上接受到的数据 htonl将主机数转换成无符号长整型的网络字节顺序
    srvAddr.sin_port =htons(port);//htons将整型变量从主机字节顺序转变网络字节顺序
    
    /*construct response address structure*/
    memset(&respAddr, 0, sizeof(respAddr));
    respAddr.sin_family = AF_INET;
    respAddr.sin_addr.s_addr = inet_addr(localAddr);
    respAddr.sin_port = htons(port);
    
    /*construct the loca DNS address structure*/
    memset(&dnsAddr, 0, sizeof(dnsAddr));
    dnsAddr.sin_family = AF_INET;
    dnsAddr.sin_addr.s_addr = inet_addr(localDNS);
    dnsAddr.sin_port =htons(port);
    
    /*bind to the local address*/
    if ((bind(sock, (struct sockaddr *) &srvAddr, sizeof(srvAddr))) < 0){
        printf("bind() failed.\n");
        exit(1);
    }
    printf("************* Successfully connect *************\n");
    
    while(1){
        /* Set the size of the in-out parameter */
        cliAddrLen = sizeof(cliAddr);
        /* Block until receive message from a client */
        recvMsgSize= recvfrom(sock, buffer, bMAX, 0,(struct sockaddr *) &cliAddr, &cliAddrLen); // receive the DNS request
        if (recvMsgSize < 0){
            printf("recvfrom() failed.\n");
        }
        else{
            /*DNS*/
            getQuery(buffer,&query); /*get the DNS query*/
            flag_ip = checkType(query.type);/*check the type of the query*/
            /*check the domain name*/
            op = flag_ip;
            if(flag_ip == 1){
                flag_existip = retIP(query.domName,ipAddr,dataSize);
                /*printf("1,type: %x\n",query.type[1]);
                 printf("2,classes: %x\n",query.classes[1]);
                 printf("ip!!: %s\n",ipAddr);*/ //测试
                op = flag_existip;
            }
            
            /*DNS relay*/
            
            /*
             * op = 0, no corresponding ip, need to send to local DNS
             * op = 1, domain name is in the database, send response back to the client
             * op = 2, domain name is not exist, send response with invaild domain name
             */
            initResp(&answer, query,ipAddr);
            switch (op) {
                case 0:
                    memcpy(&idTable.oldID , buffer, sizeof(unsigned short));
                    idTable.newID = idTable.oldID + 1;

                    memcpy(response, buffer, recvMsgSize);
                    printf("Send to local DNS\n");
                    /*retransmit the DNS request to the local DNS*/
                    memcpy(&response[0], &idTable.newID, sizeof(unsigned short));
                    rspMsgSize = recvMsgSize;
                    if(sendto(sock, response, rspMsgSize, 0,(struct sockaddr *) &dnsAddr, sizeof(dnsAddr))!=rspMsgSize){
                        printf("Send fail!\n");
                        break;
                    }
                    /* Set the size of the in-out parameter */
                    dnsAddrLen = sizeof(dnsAddr);
                    /*block until receieve the response from the local DNS*/
                    recvMsgSize= recvfrom(sock, buffer, bMAX, 0,(struct sockaddr *) &dnsAddr, &dnsAddrLen); //receive the DNS response
                    if (recvMsgSize < 0){
                        printf("recvfrom() failed.\n");
                        break;
                    }
                    printf("Get the response from local DNS\n");
                    memcpy(response, buffer, recvMsgSize);
                    memcpy(&response[0], &idTable.oldID, sizeof(unsigned short));
                    rspMsgSize = recvMsgSize;
                    /*send the response to the local client*/
                    
                    if(sendto(sock,response, rspMsgSize, 0,(struct sockaddr *) &cliAddr, sizeof(cliAddr))!=rspMsgSize){
                        printf("Send fail!\n");
                        break;
                    }
                    break;
                case 1:
                    printf("The domain name is %s\nThe ip is %s\n",query.domName,ipAddr);
                    rspMsgSize = consResp(answer, buffer, response, recvMsgSize);
                    response[2] = 0x81;//response
                    response[3] = 0x80;
                    if(sendto(sock, response, rspMsgSize, 0,(struct sockaddr *) &cliAddr, sizeof(cliAddr))!=rspMsgSize){
                        printf("Send fail!\n");
                        break;
                    }
                    /*conAnswer(ipAddr,&answer, &query,&respAddr);
                     printf("domain: %s\ntype: %x\nclass: %x\nip: %s\nlength:%x\n",answer.domName,answer.type[1],answer.classes[1],answer.addr,answer.dataLen[1]);//测试*/
                    break;
                case 2:
                    rspMsgSize = consResp(answer, buffer, response, recvMsgSize);
                    response[2] = 0x81;//response
                    response[3] = 0x83;//domain not exist
                    printf("The domain name not exist!\n");
                    if(sendto(sock, response, rspMsgSize, 0,(struct sockaddr *) &cliAddr, sizeof(cliAddr))!=rspMsgSize){
                        printf("Send fail!\n");
                        break;
                    }
                    break;
                default:
                    printf("Wrong query, try again!\n");
                    break;
            }
        }
        printf("***************************************\n");
        //printf("From client %s:%s\n",inet_ntoa(cliAddr.sin_addr),buffer); //print the DNS request
    }
    
}

/*construct response*/
int consResp(struct _DNS_RR answer,char *mesg,char *ans,int recvMsgSize){ //return message size
    int i=0;
    int pos =0; //pionter to the answer message
    int rspMsgSize;
    memcpy(&ans[pos], mesg, recvMsgSize);
    ans[6] = 0x00;
    ans[7] = 0x01;//1 answer
    i = 12;
    //printf("pos:%d\n",pos);
    pos=recvMsgSize;
    while(mesg[i]!=0x00){ /*the domain name zone*/
        ans[pos] = mesg[i];
        pos++;
        i++;
    }
    ans[pos] = 0x00;
    pos++;
    memcpy(&ans[pos],&answer.type,sizeof(unsigned short)); /*the type zone*/
    pos = pos+sizeof(unsigned short);
    memcpy(&ans[pos],&answer.classes,sizeof(unsigned short)); /*the classes zone*/
    pos = pos+sizeof(unsigned short);
    memcpy(&ans[pos],&answer.ttl,sizeof(unsigned int)); /*the ttl zone*/
    pos = pos+sizeof(unsigned int);
    memcpy(&ans[pos],&answer.dataLen,sizeof(unsigned short)); /*the data length*/
    pos = pos+sizeof(unsigned short);
    memcpy(&ans[pos],&answer.ipAddr,sizeof(unsigned int)); /*the ip address*/
    rspMsgSize = pos + sizeof(unsigned int);
    return rspMsgSize;
}
/*init response*/
void initResp(struct _DNS_RR *answer,struct _DNS_QER query,char *ip){
    answer->ttl = ntohl(0x00015180); //ttl 设置为86400s 24小时
    answer->dataLen = ntohs(0x0004); //ipv4地址长度为4
    answer->ipAddr = inet_addr(ip);
    memcpy(&answer->type, query.type, sizeof(unsigned short));
    memcpy(&answer->classes,query.type,sizeof(unsigned short));
}

/*get query form the DNS request*/
void getQuery(char *mesg, struct _DNS_QER *query){
    int qpos = 12; // query pointer, the DNS header occupied 12 bytes, then is the DNS query
    char name[68];
    int i = 0;
    while(mesg[qpos] != 0x00){ /*get the domain name*/
        if(mesg[qpos]== 0x2D || (mesg[qpos]>=0x30 && mesg[qpos]<=0x39 )||(mesg[qpos]>=0x41 && mesg[qpos]<=0x7A )) // 字符判定，属于规定的（-，0-9，a-z，A-Z）则保留
            name[i]=mesg[qpos];
        else
            name[i]= 0x2E; //判定为非规定字符则替换为‘.’
        qpos++;
        i++;
    }
    name[i] = '\0';
    memcpy(query->domName, &name[1], strlen(name));
    query->domName[i] = '\0';
    //printf("domain name :%s\n",query->domName); //检查用
    
    /*get the type of the query*/
    memcpy(query->type, &mesg[++qpos], sizeof(unsigned short));
    qpos = qpos + sizeof(unsigned short);
    /*get the classes of the query*/
    memcpy(query->classes, &mesg[qpos], sizeof(unsigned short));
}

/*check the type of the DNS request*/
int checkType(char *type){
    int op = 0;
    if(type[0]==0x00&&type[1]==0x01)
        op = 1; //IPv4
    return op;
}

/*load the local DNS database*/
int loadFile(){
    FILE *file;
    int i=0;
    file = fopen("DNSrelay.txt", "r");
    if(!file){ //check whether the file open succesffully
        printf("Database open fail");
        exit(1);
    }
    while(!feof(file)){
        fgets(data[i].string,100,file); //read on line from the database
        if(strlen(data[i].string)>=1){
            data[i].string[strlen(data[i].string)-1] = '\0';
        }
        i++;
    }
    printf("************* Database loaded successfully *************\n\n\n");
    return (i-1);
    
}


/*retrieve the ip address from the database*/
int retIP(char *domName,char *ip,int dataSize){
    int i =0;
    int flag = -1;
    int j = 0;
    for(i = 0;i<dataSize;i++){
        if(strstr(data[i].string, domName)!= NULL){ //查询ip地址是否存在，若存在
            for(j=0;data[i].string[j]!=0x20;j++) //获得IP地址
                ip[j]=data[i].string[j];
            ip[j] = '\0';
            //printf("successful!\nip: %s\n",ip);
            flag = ipVaild(ip);
            break;
        }
        else
            flag = 0;
        
    }
    //printf("flag: %d\n",flag);//测试
    return flag;
}

/*check wether the ip vaild*/
int ipVaild(char *ip){
    if(strstr(ip,"0.0.0.0")){
        ip = "0.0.0.0";
        return 2;
    }
    else
        return 1;
}
