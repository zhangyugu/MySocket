#include <WinSock2.h>
#include <Windows.h>
#include <iostream>
#include <process.h>
#include <string>
#include <MSWSock.h>
#include <set>

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Kernel32.lib")
#pragma comment(lib, "Mswsock.lib")

#define BUF_LEN 1024

enum OperateType
{
	OP_RECV,
	OP_SEND,
	OP_ACCEPT,
};

typedef struct PER_HANDLE_DATA
{
	SOCKET s;
	SOCKADDR_IN addr;
}PER_HANDLE_DATA, *LPPER_HANDLE_DATA;

typedef struct PER_IO_DATA
{
	OVERLAPPED overlapped;
	SOCKET cs;
	char buf[BUF_LEN];
	int operationType;
}PER_IO_DATA, *LPPER_IO_DATA;

SOCKET SocketInitBindListen()
{
	SOCKET s = socket(AF_INET, SOCK_STREAM, 0);
	if (INVALID_SOCKET == s)
	{
		std::cout << "create socket failed : " << GetLastError() << std::endl;
		return INVALID_SOCKET;
	}

	SOCKADDR_IN	addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.S_un.S_addr = INADDR_ANY;
	addr.sin_port = htons(4444);

	int ret = bind(s, (sockaddr*)&addr, sizeof(addr));
	if (SOCKET_ERROR == ret)
	{
		std::cout << "bind failed : " << GetLastError() << std::endl;
		return SOCKET_ERROR;
	}

	ret = listen(s, 10);
	if (SOCKET_ERROR == s)
	{
		std::cout << "listen fail : " << GetLastError() << std::endl;
		return SOCKET_ERROR;
	}

	return s;
}

bool PostAccept(SOCKET listenSocket)
{
	SOCKET cs = socket(AF_INET, SOCK_STREAM, 0);
	if (INVALID_SOCKET == cs)
	{
		std::cout << "Create Socket Failed : " << GetLastError() << std::endl;
		return false;
	}

	LPPER_IO_DATA ppiod = new PER_IO_DATA;
	ZeroMemory(&(ppiod->overlapped), sizeof(OVERLAPPED));
	ppiod->operationType = OP_ACCEPT;
	ppiod->cs = cs;

	DWORD dwRecv;
	int len = sizeof(sockaddr_in) + 16;
	bool ret = AcceptEx(listenSocket, ppiod->cs, ppiod->buf, 0, len, len, &dwRecv, &ppiod->overlapped);
	if (false == ret && ERROR_IO_PENDING != GetLastError())
	{
		std::cout << "AcceptEx Failed : " << GetLastError() << std::endl;
		return false;
	}

	return true;
}

bool PostSend(SOCKET s, const char *buf, int len)
{
	LPPER_IO_DATA ppiod = new PER_IO_DATA;
	ZeroMemory(&(ppiod->overlapped), sizeof(OVERLAPPED));
	ppiod->operationType = OP_SEND;
	memset(ppiod->buf, 0, BUF_LEN);
	memcpy(ppiod->buf, buf, len);

	WSABUF databuf;
	databuf.buf = ppiod->buf;
	databuf.len = len;

	DWORD dwRecv = 0;
	DWORD dwFlags = 0;
	int ret = WSASend(s, &databuf, 1, &dwRecv, dwFlags, &ppiod->overlapped, NULL);
	if (SOCKET_ERROR == ret && WSA_IO_PENDING != GetLastError())
		return false;

	return true;
}

/****************************************************
Client中收发顺序：发-收-收-发
这个子线程证明了线程A在socket 1上投递异步接收请求后等待数据到来，线程B在socket 1上投递异步发送请求，两厢互不干扰。
*****************************************************/
unsigned int __stdcall Func(void *arg)
{
	SOCKET s = (SOCKET)arg;

	while (std::cin.get())
	{
		std::string str = "nihaihaoma";
		bool ret = PostSend(s, str.c_str(), str.length());
		if (false == ret)
			break;
	}

	_endthreadex(0);
	return 0;
}

unsigned int __stdcall ThreadFunc(void *arg)
{
	HANDLE hcp = (HANDLE)arg;
	if (NULL == hcp)
	{
		std::cout << "thread arg error" << std::endl;
		return -1;
	}

	DWORD dwNum = 0;
	LPPER_HANDLE_DATA pphd;
	LPPER_IO_DATA ppiod;
	while (true)
	{
		bool ret = GetQueuedCompletionStatus(hcp, &dwNum, (LPDWORD)&pphd, (LPOVERLAPPED*)&ppiod, WSA_INFINITE);

		//线程退出控制，没有释放申请的堆空间，还不完善
		if (-1 == dwNum)
		{
			std::cout << "Thread Exit" << std::endl;
			_endthreadex(0);
			return 0;
		}

		/*************************************************************************
		实际试验结果如下：
			前提：Server在s已经投递WSARecv，WSARecv此时正等待数据到来

			若Client调用closesocket优雅关闭连接，
			则该WSARecv在GetQueuedCompletionStatus中以ret=true，dwNum=0，GetLastError()=997返回 （997，WSA_IO_PENDING）

			若Client直接退出，
			则该WSARecv在GetQueuedCompletionStatus中以ret=false，dwNum=0，GetLastError()=64返回 （64，指定的网络名不再可用）

			若Server调用closesocket关闭该socket，
			则该WSARecv在GetQueuedCompletionStatus中以ret=false，dwNum=0，GetLastError()=64返回 （64，指定的网络名不再可用）

			若Client突然断电，
			则2014.7.23 18:49 用丁胖子电脑测试结果表明对于暴力断电，IOCP无任何反应，故心跳检测必不可少

		注：以上结论均是反复验证的结果，故可靠性较高
		****************************************************************************/
		//连接断开
		int type = ppiod->operationType;
		if (0 == dwNum && OP_ACCEPT != type)
		{
			std::cout << "The Connection Be Closed : " << GetLastError() << std::endl;
			closesocket(pphd->s);
			delete pphd;
			delete ppiod;
			continue;
		}

		//错误发生
		if (false == ret)
		{
			std::cout << "An Error Occurs : " << GetLastError() << std::endl;
			closesocket(pphd->s);
			delete pphd;
			delete ppiod;
			continue;
		}

		if (OP_RECV == type)
		{
			//
			std::cout << "接收完成" << std::endl;
			std::cout << "接收端口号 ：" << pphd->s << std::endl;
			//

			ppiod->buf[dwNum] = '\0';
			std::cout << ppiod->buf << std::endl;

			ZeroMemory(&(ppiod->overlapped), sizeof(OVERLAPPED));
			ZeroMemory(ppiod->buf, BUF_LEN);
			WSABUF databuf;
			databuf.buf = ppiod->buf;
			databuf.len = BUF_LEN;

			DWORD dwRecv = 0;
			DWORD dwFlags = 0;
			WSARecv(pphd->s, &databuf, 1, &dwRecv, &dwFlags, &ppiod->overlapped, NULL);
		}
		else if (OP_SEND == type)
		{
			//
			std::cout << "发送完成" << std::endl;
			std::cout << "完成发送的端口号 : " << pphd->s << std::endl;
			//

			delete ppiod;
		}
		else if (OP_ACCEPT == type)
		{
			//
			std::cout << "连接完成" << std::endl;
			//

			SOCKET cs = ppiod->cs;
			int len = sizeof(sockaddr_in) + 16;
			int localLen, remoteLen;
			LPSOCKADDR localAddr, remoteAddr;
			GetAcceptExSockaddrs(ppiod->buf, 0, len, len, (SOCKADDR **)&localAddr, &localLen, (SOCKADDR **)&remoteAddr, &remoteLen);

			LPPER_HANDLE_DATA p = new PER_HANDLE_DATA;
			p->s = cs;
			memcpy(&p->addr, remoteAddr, remoteLen);
			char *ch = inet_ntoa(p->addr.sin_addr);

			CreateIoCompletionPort((HANDLE)cs, hcp, (DWORD)p, 0);

			ZeroMemory(&(ppiod->overlapped), sizeof(OVERLAPPED));
			ppiod->operationType = OP_RECV;
			WSABUF databuf;
			databuf.buf = ppiod->buf;
			databuf.len = BUF_LEN;

			DWORD dwRecv = 0;
			DWORD dwFlags = 0;
			WSARecv(p->s, &databuf, 1, &dwRecv, &dwFlags, &ppiod->overlapped, NULL);

			PostAccept(pphd->s);
			PostAccept(pphd->s);

			//测试在s等待接收时，另一线程在s上投递发送会怎样
			_beginthreadex(NULL, 0, Func, (void*)p->s, 0, NULL);
		}
	}

	return 0;
}

int main5()
{
	WSADATA ws;
	if (WSAStartup(MAKEWORD(2, 2), &ws) != 0)
	{
		std::cout << "WSAStartup error : " << GetLastError() << std::endl;
		return -1;
	}

	HANDLE hcp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (NULL == hcp)
	{
		std::cout << "create completion port failed : " << GetLastError() << std::endl;
		return -1;
	}

	std::set<HANDLE> setWorkers;
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	for (int i = 0; i < si.dwNumberOfProcessors * 2 + 2; i++)
	{
		HANDLE worker = (HANDLE)_beginthreadex(NULL, 0, ThreadFunc, (LPVOID)hcp, 0, NULL);
		if (NULL == worker)
		{
			std::cout << "create thread failed : " << GetLastError() << std::endl;
			return -1;
		}
		setWorkers.insert(worker);
	}

	SOCKET s = SocketInitBindListen();
	if (INVALID_SOCKET == s)
	{
		std::cout << "socket init failed" << std::endl;
		return -1;
	}

	LPPER_HANDLE_DATA pphd = new PER_HANDLE_DATA;
	pphd->s = s;
	CreateIoCompletionPort((HANDLE)s, hcp, (DWORD)pphd, 0);

	bool ret = PostAccept(s);
	if (false == ret)
	{
		std::cout << "PostAccept Failed" << std::endl;
		return -1;
	}

	//退出控制
	/*std::cin.get();
	for(int i = 0; i < setWorkers.size(); i++)
		PostQueuedCompletionStatus(hcp, -1, NULL, NULL);*/

	auto iter = setWorkers.begin();
	for (; iter != setWorkers.end(); iter++)
		WaitForSingleObject(*iter, INFINITE);

	WSACleanup();

	std::cin.get();
	return 0;
}