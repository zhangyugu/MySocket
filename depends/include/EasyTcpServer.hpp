#ifndef EASY_TCP_SERVER_HPP
#define EASY_TCP_SERVER_HPP
#include "GlobalDef.hpp"
#include"CellServer.hpp"
#include "SockContext.hpp"

class EasyTcpServer: public INetEvent
{
private:
	CellThread _therad;
	std::vector<std::shared_ptr<CellServer>> _vce_pcellserver;
	CELLTimestamp _rTime;
	std::mutex _mutex;
protected:
	SOCKET _sock;
	std::atomic_int _OnConut;//消息计数
	std::atomic_int _OnRevc;//接收计数
	std::atomic_int _clientCount;//客户计数
protected:
	//等待客户连接
	virtual bool OnRun(CellThread *pthread) = 0;
public:
	EasyTcpServer(): _sock(INVALID_SOCKET), _OnConut(0), _clientCount(0), _OnRevc(0)
	{	
		initSocket();
	}
	virtual ~EasyTcpServer()
	{
		Close();
	}
	//3 listen 监听网络端口
	bool Listen(int i = 5)
	{
		if (SOCKET_ERROR == listen(_sock, i))
		{
			Log::Info("错误, <_sock=%d>监听端口失败...\n",_sock);
			return false;
		}

		Log::Info("监听成功<_sock=%d>等待客户端加入...\n", _sock);
		return true;
	}
	//2 bind 绑定用于接收客户端连接的网络端口
	bool BindPort(unsigned short post, unsigned short family = AF_INET, const char* addr = nullptr)
	{
		if (INVALID_SOCKET == _sock)//未启动
		{
			if (!initSocket())
			{
				return false;
			}
		}

		sockaddr_in _sin = {};
		_sin.sin_family = family;//地址规范  IPv4 IPv6
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
			Log::Info("错误, 绑定端口<post=%d>失败...\n", post);
			return false;
		}

		return true;
	}

	template<typename Server>
	void Start(int TCount = 1)
	{
		if (TCount < 1)
			TCount = 1;

		for (int i = 0; i < TCount; ++i)
		{
			//创建处理对象
			auto tmp = new Server(this);
			if (tmp)
			{
				_vce_pcellserver.push_back(std::shared_ptr<CellServer>(tmp));
				//启动处理线程
				tmp->Start();
			}
		}

		_therad.Start([this](CellThread *pthread) { OnRun(pthread); });
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

private:
	//初始化 1 建立一个 socket 套接字
	bool initSocket()
	{
		SockContext::Instance();
		//--用 Socket API 建立简易的TCP服务端
		if (INVALID_SOCKET != _sock)
			CloseSock(_sock);

		 _sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);//地址规范, 类型, 协议
		 if (INVALID_SOCKET == _sock)
		 {
			 Log::Info("错误, 建立套接字失败...\n");
			 return false;
		 }

		 return true;
	}

	
	//添加客户端到CellServer
	bool addClient(ClientInfo* pClient)
	{
		if (pClient == nullptr)
			return false;

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

	
protected:
	//accept 等待接受客户端连接
	bool Accept()
	{

		ClientInfo* client = new ClientInfo;//客户端传进来的套接字
		if (client == nullptr)
		{
			Log::Info("错误, 无法为客户端分配内存...\n");
			return false;
		}
#ifdef _WIN32
		int clientlen = sizeof(sockaddr_in);//客户端传进来的套接字的长度
#else
		unsigned int clientlen = sizeof(sockaddr_in);//客户端传进来的套接字的长度
#endif
		client->_socket = accept(_sock, (sockaddr*)&client->addr, &clientlen);
		if (INVALID_SOCKET == client->_socket)
		{
			Log::Info("错误, 接受到无效客户端套接字...\n");
			delete client;
			return false;
		}

		//IP = inet_ntoa(client->addr.sin_addr) ;

		//添加新的客户端
		addClient(client);

		return true;
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