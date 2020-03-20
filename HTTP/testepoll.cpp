#include <stdio.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#define MAX_EVENTS 1024 /*监听上限*/
#define BUFLEN  4096    /*缓存区大小*/
#define SERV_PORT 4567  /*端口号*/
#define LISTENQ 1024
#define MAXSIZE 4096
#define POLL_SIZE 1024

struct DataHeader
{
	unsigned short datalength;//数据长度
	unsigned short cmd;//命令
	//char a[96];
};
int make_socket_non_blocking(int fd) {
	int flag = fcntl(fd, F_GETFL, 0);
	if (flag == -1)
		return -1;

	flag |= O_NONBLOCK;
	if (fcntl(fd, F_SETFL, flag) == -1)
		return -1;
	return 0;
}
int socket_bind_listen(int port) {
	// 检查port值，取正确区间范围
	port = ((port <= 1024) || (port >= 65535)) ? 6666 : port;

	// 创建socket(IPv4 + TCP)，返回监听描述符
	int listen_fd = 0;
	if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		return -1;

	// 消除bind时"Address already in use"错误
	int optval = 1;
	if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, (const void*)&optval, sizeof(int)) == -1) {
		return -1;
	}

	// 设置服务器IP和Port，和监听描述副绑定
	struct sockaddr_in server_addr;
	bzero((char*)&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons((unsigned short)port);
	if (bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
		return -1;

	// 开始监听，最大等待队列长为LISTENQ
	if (listen(listen_fd, LISTENQ) == -1)
		return -1;

	// 无效监听描述符
	if (listen_fd == -1) {
		close(listen_fd);
		return -1;
	}

	return listen_fd;
}
int main()
{
	int listener, nfds, n;
	int port = SERV_PORT;
	char buffer[MAXSIZE] = {};

	//初始化监听socket
	if ((listener = socket_bind_listen(port)) < 0)
	{
		printf((char*)"create socket in %s err %s\n", __func__, strerror(errno));
		exit(-1);
	}
	make_socket_non_blocking(listener);
	sockaddr_in local;
	int clientlen = sizeof(sockaddr_in), client;//客户端传进来的套接字的长度
	int epfd = epoll_create(POLL_SIZE);
	if (epfd <= 0)
	{
		printf((char*)"创建epoll失败\n");
		return -1;
	}
	struct epoll_event ev;
	ev.data.fd = listener;
	ev.events = EPOLLIN | EPOLLET;
	if (epoll_ctl(epfd, EPOLL_CTL_ADD, listener, &ev) < 0)
	{
		printf((char*)"添加本地端口失败\n");
		return -1;
	}
		
	epoll_event *events = (epoll_event *)malloc(POLL_SIZE * sizeof(epoll_event));
	if (!events)
		{
			printf((char*)"分配内存失败\n");
			return -1;
		}
	while (1)
	{
		nfds = epoll_wait(epfd, events, 20, 500);
		{
			for (n = 0; n < nfds; ++n) {
				if (events[n].data.fd == listener) {
					//如果是主socket的事件的话，则表示
					//有新连接进入了，进行新连接的处理。
					client = accept(listener, (struct sockaddr *)&local, (socklen_t *)&clientlen);
					if (client < 0) {
						perror((char*)"accept");
						continue;
					}
					make_socket_non_blocking(client);        //将新连接置于非阻塞模式
					ev.events = EPOLLIN | EPOLLET; //并且将新连接也加入EPOLL的监听队列。
					//注意，这里的参数EPOLLIN|EPOLLET并没有设置对写socket的监听，
					//如果有写操作的话，这个时候epoll是不会返回事件的，如果要对写操作
					//也监听的话，应该是EPOLLIN|EPOLLOUT|EPOLLET
					ev.data.fd = client;
					printf((char*)"添加客户端\n");
					if (epoll_ctl(epfd, EPOLL_CTL_ADD, client, &ev) < 0) {
						//设置好event之后，将这个新的event通过epoll_ctl加入到epoll的监听队列里面，
						//这里用EPOLL_CTL_ADD来加一个新的epoll事件，通过EPOLL_CTL_DEL来减少一个
						//epoll事件，通过EPOLL_CTL_MOD来改变一个事件的监听方式。
						//fprintf(stderr, (char*)"epollsetinsertionerror:fd=%d", client);
						printf((char*)"添加客户端失败\n");
						return -1;
					}
				}
				else if (events[n].events & EPOLLIN)
				{
					printf((char*)"读事件\n");
					//如果是已经连接的用户，并且收到数据，
					//那么进行读入
					int sockfd_r, rlen;
					if ((sockfd_r = events[n].data.fd) < 0)
						continue;
					if ((rlen = read(sockfd_r, buffer, MAXSIZE)) > 0)
					{
						printf((char*)"收到:");
						for (int i = 0; i < rlen; ++i)
							printf((char*)"%c", buffer[i]);
						printf((char*)"\n");
					}						
					//修改sockfd_r上要处理的事件为EPOLLOUT
					ev.data.fd = sockfd_r;
					ev.events = EPOLLOUT | EPOLLET;
					epoll_ctl(epfd, EPOLL_CTL_MOD, sockfd_r, &ev);
				}
				else if (events[n].events & EPOLLOUT)
				{
					printf((char*)"写事件\n");
					//如果有数据发送
					int sockfd_w = events[n].data.fd;
					DataHeader header;
					header.cmd = 8;
					header.datalength = sizeof(DataHeader) + sizeof(char);
					char buf[32], status = 0;
					memcpy(buf, &header, sizeof(DataHeader));
					memcpy(buf + sizeof(DataHeader), &status, sizeof(char));
					write(sockfd_w, buf, header.datalength);
					//修改sockfd_w上要处理的事件为EPOLLIN
					ev.data.fd = sockfd_w;
					ev.events = EPOLLIN | EPOLLET;
					epoll_ctl(epfd, EPOLL_CTL_MOD, sockfd_w, &ev);
				}
			}
		}
	}
	
	return 0;
}
