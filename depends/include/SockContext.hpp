#ifndef SOCKET_CONTEXT_HPP
#define SOCKET_CONTEXT_HPP
#ifdef _WIN32
#ifndef FD_SETSIZE
#define FD_SETSIZE 5000
#endif // !FD_SETSIZE
#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <WinSock2.h>
#include <MSWSock.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <algorithm>
#include <signal.h>
#include <sys/epoll.h>
#include "CellEpoll.hpp"
typedef char* PCHAR;
using SOCKET = int;
#define INVALID_SOCKET  (SOCKET)(~0)
#define SOCKET_ERROR            (-1)
#endif

#include <iostream>

//sock 环境初始化
class SockContext
{
public:
	static void Instance()
	{
		static SockContext obj;
		//if (!obj)
		//	obj = new SockContext;
	}
private:
	//static SockContext *obj;
private:
	SockContext() 
	{
#ifdef _WIN32
		WSADATA dat;
		//启动windows socket 2.x环境
		if (NO_ERROR != WSAStartup(MAKEWORD(2, 2), &dat)) {
			printf("CellSockContext.WSAStartup failed!\n");
		}
#else
		//if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
			//return (1);
		//忽略异常信号, 不退出进程
		signal(SIGPIPE, SIG_IGN);
#endif
	}
	~SockContext() {
		/*if (obj)
			delete obj;*/
#ifdef _WIN32
		WSACleanup();//清除windows socket环境
#endif	
	}
};

#endif //!SOCKET_CONTEXT_HPP