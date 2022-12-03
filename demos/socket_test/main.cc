/**************************************************************************
 *
 * Copyright (c) 2015 Cheeray Huang. All Rights Reserved
 *
 * @file: main.cc
 * @author: Huang Qiyu
 * @email: cheeray.huang@gmail.com
 * @date: 11-30-2022 23:16:40
 * @version $Revision$
 *
 **************************************************************************/


#include<stdio.h>
#include<arpa/inet.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#define BUFFER_SIZE 512
//接受缓冲区设置为50，发送缓冲区设置为2000
int main(int argc,char *argv[]){
    //1.创建套接字
    int fd = socket(AF_INET,SOCK_STREAM,0);
    if(fd == -1){
        perror("socket");
        exit(0);
    }
    //设置发送缓冲区的大小。
    int sendbuf = 2000;
    int len = sizeof(sendbuf);
    //setsockopt(fd,SOL_SOCKET,SO_SNDBUF,&sendbuf,sizeof(sendbuf));
    getsockopt(fd,SOL_SOCKET,SO_SNDBUF,&sendbuf,(socklen_t*)&len);
    printf("the tcp send buffer size after setting is %d\n",sendbuf);

    getsockopt(fd,SOL_SOCKET,SO_RCVBUF,&sendbuf,(socklen_t*)&len);
    printf("the tcp recv buffer size after setting is %d\n",sendbuf);


    //2.连接服务器
    /*
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    inet_pton(PF_INET,"192.168.131.137",&server_addr.sin_addr);
    server_addr.sin_port = htons(9999);
    int ret = connect(fd,(struct sockaddr*)&server_addr,sizeof(server_addr));
    if(ret == -1){
        perror("connect");
        exit(0);
    }
    char buffer[BUFFER_SIZE];
    memset(buffer,'a',BUFFER_SIZE);
    send(fd,buffer,BUFFER_SIZE,0);
    */
    close(fd);

    return 0;
}
