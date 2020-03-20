#ifndef EASY_TCP_SERVER_HPP
#define EASY_TCP_SERVER_HPP
#include "GlobalDef.hpp"
#include"CellServer.hpp"
#include "SockContext.h"


class EasyTcpServer: public INetEvent
{
private:
	CellThread _therad;//主线程  任务: 接收客户端连接
	std::vector<std::shared_ptr<CellServer>> _vce_pcellserver;//副线程组 任务: 处理消息、发送消息
	SOCKET _sock = INVALID_SOCKET;//主套接字
	fd_set _fdRead{};//可读套接字描述符
	fd_set _fdWrite{};//可写套接字描述符
	fd_set _fdExp{};//错误套接字描述符
	CELLTimestamp _rTime;//计时
#ifdef _WIN32
	IOCP _iocpListen;
#endif
protected:
	std::atomic_int _OnConut{};//消息计数
	std::atomic_int _OnRevc{};//接收计数
	std::atomic_int _clientCount{};//客户计数
	std::mutex _mutex;//线程锁
public:
	EasyTcpServer()
	{	
		
	}
	virtual ~EasyTcpServer()
	{
		Close();
	}
		
	void Start()
	{		
		for (int i = 0; i < /*std::thread::hardware_concurrency()*/1; ++i)
		{
			//创建处理对象
			auto tmp = new CellServer(this);
			if (tmp)
			{
				_vce_pcellserver.push_back(std::shared_ptr<CellServer>(tmp));
				//启动处理线程
				tmp->Start();
			}
		}

		//_therad.Start([this](CellThread *pthread) { OnRunLoop(pthread); });

	}
	//9 关闭
	void Close()
	{

		if (INVALID_SOCKET != _sock)
		{
			_therad.Close();
			_vce_pcellserver.clear();
			CloseSock(_sock);
			//8 关闭 "socket" closesocket
		}
	}

	//初始化 1 建立一个 socket 套接字
	bool CreateListenSocket(unsigned short post, int i = 1024, const char* addr = nullptr)
	{
		SockContext::Instance();

#ifdef _WIN32
		return _iocpListen.CreateListenSocket([this](SOCKET &socket, sockaddr_in &addr) {
			ClientInfo* client = new ClientInfo;//客户端传进来的套接字	
			if (client == nullptr)
			{
				Log::Info("error, Unable to allocate memory for client...\n");
				return;
			}
			client->_socket = socket;
			memcpy(&client->_addr, &addr, sizeof(sockaddr_in));
			if (!addClient(client))
				delete client;

		}, post, i);
#else

		//--用 Socket API 建立简易的TCP服务端
		if (INVALID_SOCKET != _sock)
			CloseSock(_sock);

		_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);//地址规范, 类型, 协议
		if (INVALID_SOCKET == _sock)
		{
			Log::Info("error, Failed to establish socket...\n");
			return false;
		}

		sockaddr_in _sin = {};
		_sin.sin_family = AF_INET;//地址规范  IPv4 IPv6
		_sin.sin_port = htons(post);//端口   由于不同系统的USHORT字节长度不同, 需要统一转换
		auto ip = addr ? inet_addr(addr) : INADDR_ANY;
#ifdef _WIN32
		_sin.sin_addr.S_un.S_addr = ip;//绑定本机ip地址  INADDR_ANY 本机的全部地址  //inet_addr("127.0.0.1");
#else
		_sin.sin_addr.s_addr = ip;
#endif
		if (SOCKET_ERROR == bind(_sock, (sockaddr*)&_sin, sizeof(sockaddr_in)))
		{
			//端口被占用
			Log::Info("error, post<%d> bind failed...\n", post);
			return false;
		}
		if (SOCKET_ERROR == listen(_sock, i))
		{
			Log::Info("error, socket<%d> listen failed...\n", _sock);
			return false;
		}

		Log::Info("listen socket<%d> success, Wait for the client to join...\n", _sock);
#endif
		

		//#ifndef _WIN32
		//		 make_socket_non_blocking(_sock);
		//#endif
		return true;
	}
	

private:
	//accept 等待接受客户端连接
	bool Accept()
	{
		
		if (_clientCount <= _vce_pcellserver.size() * MAX_CLIENT)
		{
			ClientInfo* client = new ClientInfo;//客户端传进来的套接字
			if (client == nullptr)
			{
				Log::Info("error, Unable to allocate memory for client...\n");
				return false;
			}
#ifdef _WIN32
			int clientlen = sizeof(sockaddr_in);//客户端传进来的套接字的长度
#else
			unsigned int clientlen = sizeof(sockaddr_in);//客户端传进来的套接字的长度
#endif
			client->_socket = accept(_sock, (sockaddr*)&client->_addr, &clientlen);
			if (INVALID_SOCKET == client->_socket)
			{
				Log::Info("error, Invalid client socket received...\n");
				delete client;
				return false;
			}
			//IP = inet_ntoa(client->addr.sin_addr) ;
	//#ifndef _WIN32
	//		make_socket_non_blocking(client->sock);
	//		client->_event.data.fd = client->sock;
	//		client->_event.events = EPOLLIN | EPOLLOUT | EPOLLET | EPOLLONESHOT;
	//#endif // !_WIN32

			//添加新的客户端
			addClient(client);		
			return true;
		}
		return false;
	}


	//添加客户端到CellServer
	bool addClient(ClientInfo* pClient)
	{
		if (pClient && !_vce_pcellserver.empty())
		{				
			pClient->_dtconnect_login = CELLTime::getNowinMilliSec();//连接时间
			//保存到数量最少的线程
			auto MaxSize = _vce_pcellserver.front();
			for (auto &x : _vce_pcellserver)
			{
				if (MaxSize->getClientCount() > x->getClientCount())
					MaxSize = x;
			}

			MaxSize->addClient(pClient);
			
		
			return true;
		}
		else
			return false;
	}

	//等待客户连接
	bool OnRunLoop(CellThread *pthread)
	{
		while (pthread->isRun())
		{
			calcRecv();
			FD_ZERO(&_fdRead);//清空
			FD_SET(_sock, &_fdRead);//设置
		
			//nfds 是一个整数值, 是指fd_set集合中所有描述符(socket)的范围,
			//而不是数量.既是所有文件描述符中最大值+1, 再windows中这个参数可以写0
			//timeout为NULL时无限等待事件产生
			timeval timeout = { 0,1000 };
			int ret = select((int)_sock + 1, &_fdRead, nullptr, nullptr, &timeout);
			if (ret < 0)
			{
				Log::Info("TcpServer.OnRunLoop.select Error!\n");
				break;
			}
			else if (ret == 0)
				continue;
#ifdef _WIN32
			if (_sock == *_fdRead.fd_array)//检查
#else
			if (FD_ISSET(_sock, &_fdRead))//检查
#endif
			{
				//FD_CLR(_sock, &_fdRead);//删除
				Accept();//接受客户端连接
				
			}
		}
		
		

		return false;
	}

	//关闭Sock
	inline void CloseSock(SOCKET &sock)
	{
		//8 关闭 "socket" closesocket
#ifdef _WIN32
		closesocket(sock);
#else
		close(sock);
#endif
		sock = INVALID_SOCKET;
	}

	bool calcRecv()
	{
		if (_clientCount <= 0)
			return false;

		auto t1 = _rTime.getElapsedSecond();
		if (t1 >= 1.000000)
		{
			Log::Info("thread<%d>,time<%lf>,client<%d>,OnRevc<%d>,OnCount<%d>\n",
				(int)_vce_pcellserver.size(),
				t1,
				(int)_clientCount,
				(int)_OnRevc,
				(int)(_OnConut / t1));
			_OnRevc = 0;
			_OnConut = 0;
			_rTime.update();
		}
		return true;
	}
protected:
	//只被一个线程调用  称为线程安全函数
	// 被多个线程调用   称为线程不安全函数
	virtual void OnNetLeave(ClientInfo* pClient)
	{
		//执行处理
		--_clientCount;
	}
	virtual void OnNetJoin(ClientInfo* pClient)
	{
		//执行处理
		++_clientCount;
	}
	virtual void OnNetRevc(ClientInfo* pClient)
	{
		++_OnRevc;
	}
	virtual void OnNetMsg(CellServer *pCellServer, ClientInfo* pClient)
	{
		++_OnConut;
	}
	
};

#endif