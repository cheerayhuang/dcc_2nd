/**************************************************************************
 *
 * Copyright (c) 2015 Cheeray Huang. All Rights Reserved
 *
 * @file: client.cc
 * @author: Huang Qiyu
 * @email: cheeray.huang@gmail.com
 * @date: 11-29-2022 10:49:43
 * @version $Revision$
 *
 **************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>

#include <iostream>

#define IP "192.168.102.174"
#define PORT 7000

typedef struct data {
	char name[30];
	//unsigned int num;
}Data;

void print_err(const char *str, int line, int err_no) {
	printf("%d, %s :%s\n",line,str,strerror(err_no));
	_exit(-1);
}
int skfd = -1;
void *receive(void *pth_arg) {
	int ret = 0;
	Data stu_data = {0};
	while(1) {
		bzero(&stu_data, sizeof(stu_data));
		ret = recv(skfd, &stu_data, sizeof(stu_data),0);
		if (-1 == ret) {
			print_err("recv failed",__LINE__,errno);
		}
		else if (ret > 0)
			printf("student name = %s\n",stu_data.name);

	}
}

void sig_fun(int signo) {
	if (signo == SIGINT) {
		//close(cfd);
		shutdown(skfd,SHUT_RDWR);
		_exit(0);
	}
}

int main()
{
	int ret = -1;
	skfd = socket(AF_INET, SOCK_STREAM, 0);
	if ( -1 == skfd) {
		print_err("socket failed",__LINE__,errno);
	}

    struct sockaddr_in local_addr;
    local_addr.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &local_addr.sin_addr);
    local_addr.sin_port = htons(10049);
    bind(skfd, reinterpret_cast<sockaddr*>(&local_addr), sizeof(local_addr));

    std::cout << "skfd: " << skfd << std::endl;

	/*这里需要填写服务器的端口号和IP地址，此时客户端会自己分配端口号和自己的IP发送给服务器*/
	struct sockaddr_in addr;
	addr.sin_family = AF_INET; //设置tcp协议族
	addr.sin_port = htons(8070); //设置服务器端口号
	addr.sin_addr.s_addr = inet_addr("127.0.0.1"); //设置服务器ip地址，如果是跨网，则需要填写服务器所在路由器的公网ip
	ret = connect(skfd,(struct sockaddr *)&addr,sizeof(addr));
	if ( -1 == ret ) {
		print_err("connect failed", __LINE__, errno);
	}

	pthread_t id;
	pthread_create(&id,NULL,receive,NULL);

	Data std_data = {0};
	while (1) {
		bzero(&std_data, sizeof(std_data));
		printf("stu name:\n");
		scanf("%s",std_data.name);


        /*
		printf("stu num:\n");
		scanf("%d",&std_data.num);
		std_data.num = htonl(std_data.num);
        */

		ret = send(skfd, (void *)&std_data,sizeof(std_data),0);
		if ( -1 == ret) {
			print_err("send failed", __LINE__, errno);
		}
	}


	return 0;
}
