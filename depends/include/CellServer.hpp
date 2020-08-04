#ifndef CELL_SERVER_HPP
#define CELL_SERVER_HPP
#include"GlobalDef.hpp"
#include"INetEvent.hpp"
#include"ClientInfo.hpp"
#include"CellTaskServer.hpp"


class CellServer
{
private:
	std::atomic_int ClientCount;//客户端数量
	std::map<SOCKET, std::shared_ptr<ClientInfo>> _vce_pclients;//客户队列
	std::queue<std::shared_ptr<ClientInfo>> _vce_pclientsBuff;//缓冲客户队列
	bool _fdRead_change;//客户端数量是否有改变
#ifdef _WIN32
	IOCP iocp;
#else
	CellEpoll epoll;
#endif // WIN32
	
	
	std::mutex _mutex;//缓冲区线程锁
	INetEvent *_INetEvent;//事件
	CellTaskServer _taskServer;//任务处理模块	
	time_t _old_time;
	CellThread _thread;//主线程 任务: 接收 发送 处理

public:
	CellServer(INetEvent *eventObj = nullptr) : 
		_INetEvent(eventObj), ClientCount(0), _fdRead_change(true)
#ifndef _WIN32
		,epoll(MAX_CLIENT)
#endif // WIN32
	{

	}
	virtual ~CellServer()
	{
		Close();
	}


	bool OnRunLoop(CellThread *thread)
	{
		
#ifdef _WIN32
		/*fd_set fdRead, fdRead_back, fdWrite, fdExp;
		SOCKET Max = 0;*/
		iocp.Start();
		printf("listen succeed, Wait for the client to join...\n");
		iocp.SetDeleteSocket([this](void * client) {
			KickClient(static_cast<ClientInfo*>(client));
		});
		iocp.SetRecvBackCall(/*[](void * client) {}*/nullptr,[this](void * client) {
			ClientInfo* pClient = static_cast<ClientInfo*>(client);
			while (pClient->hasMsg())
			{
				//处理
				/*_taskServer.addTask([this, &pinfo]() {
				});*/
				OnNetMsg(pClient);
				pClient->pop_front();
			}
		});
		/*iocp.SetSendBackCall(
			[](void * client) {},[](void * client) {});*/
#else
		if (!epoll.isNormal())
		{
			printf("epoll未正常启动!\n");
			return false;
		}
#endif


		_old_time = CELLTime::getNowinMilliSec();		
		while (thread->isRun())
		{
			
#ifndef _WIN32	
			if (!_vce_pclientsBuff.empty())
			{
				//从缓冲区取出客户数据
				std::lock_guard<std::mutex> lg(_mutex);
				while (!_vce_pclientsBuff.empty())
				{
					auto &x = _vce_pclientsBuff.front();			
					if (epoll.add(x->sock) < 0)//EPOLLIN | EPOLLOUT | EPOLLET | EPOLLONESHOT注册客户端;					
						printf("添加socket<%d>失败\n", x->_socket);
					else
						_vce_pclients.insert({ x->_socket, x });
									
					_vce_pclientsBuff.pop();
				}
				 
				_fdRead_change = true;
			}
#endif	
			//没有客户端
			if (ClientCount <= 0)/*_vce_pclients.empty()*/
			{
				//休眠 1 毫秒				 				
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
				_old_time = CELLTime::getNowinMilliSec();
				continue;
			}

			

			//printf("client<%zd>\n", _vce_pclients.size());
			//for (auto & x : _vce_pclients)
			//{
			//	//是否有数据可处理
			//	while (x.second->hasMsg())
			//	{
			//		//处理
			//		/*_taskServer.addTask([this, &pinfo]() {
			//		});*/
			//		OnNetMsg(x.second.get());
			//		x.second->pop_front();
			//	}
			//	x.second->SendDateReal();
			//}
			//if (_fdRead_change)
			//{
			//	_fdRead_change = false;
			//	FD_ZERO(&fdRead);//清空
			//	FD_ZERO(&fdWrite);
			//	FD_ZERO(&fdExp);
			//	//将所有客户机添加到集合
			//	Max = _vce_pclients.begin()->first;//最大值， 用于 select
			//	for (auto x : _vce_pclients)
			//	{
			//		FD_SET(x.first, &fdRead);
			//		if (Max < x.first)
			//			Max = x.first;
			//	}
			//	memcpy(&fdRead_back, &fdRead, sizeof(fd_set));	
			//}
			//else
			//	memcpy(&fdRead, &fdRead_back, sizeof(fd_set));
			//memcpy(&fdWrite, &fdRead_back, sizeof(fd_set));
			//memcpy(&fdExp, &fdRead_back, sizeof(fd_set));
			////nfds 是一个整数值, 是指fd_set集合中所有描述符(socket)的范围,
			////而不是数量.既是所有文件描述符中最大值+1, 再windows中这个参数可以写0
			////timeout为NULL时无限等待事件产生
			//timeval t{ 0, 1 };
			//if (select((int)Max + 1, &fdRead, &fdWrite, &fdExp, &t) < 0)
			//{
			//	Log::Info("CellServer.OnRunLoop.select Error!\n");
			//	break;	
			//}
			////执行处理
			//ReadData(fdRead);
			//WriteData(fdWrite);
			////fdRead.fd_array + FD_SETSIZE != std::find(fdRead.fd_array, fdRead.fd_array + FD_SETSIZE, (*it)->sock)
			//if (fdExp.fd_count > 0)
			//{
			//	Log::Info("###CellServer.OnRunLoop fdExp = %d\n", fdExp.fd_count);
			//}

#ifdef _WIN32
			//iocp.PostSend(_vce_pclients.begin()->second.get());
			printf("\nClient<%d>\n", (int)ClientCount);
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
			
#else
			if (epoll.wait(1, [this](int fd) {					
				auto it = _vce_pclients.find(fd);				
				if (!RecvData(it->second.get()))
				{
					//Log::Info("###CellServer.ReadData fSock = %d\n", (int)it->second->sock);
					RemoveClient(it->second.get());					
					it = _vce_pclients.erase(it);//在容器循环中删除其元素
				}
				},
				 [this](int fd) {
					//printf("可写事件socket<%d>\n", fd);
				auto it = _vce_pclients.find(fd);
				if (SOCKET_ERROR == it->second->SendDateReal())
				{
					//printf("###CellServer.WriteData fSock = %d\n", (int)it->second->sock);
					//Log::Info("###CellServer.WriteData fSock = %d\n", (int)it->second->sock);
					RemoveClient(it->second.get());				
					_vce_pclients.erase(it);//在容器循环中删除其元素					
				}
			    }
				) < 0)
			{
				Log::Info("CellServer.OnRunLoop.epoll Error!\n");
				break;
			}
					
#endif			

			CheckTime();//检查超时
		}

		//关闭所有客户端的套接字
		/*
			stl容器的clear() erase() 会调用元素的析构函数
			shared_ptr析构会调用delete 释放其保存的指针
			delete会调用ClientInfo的析构关闭sock
		*/
		_vce_pclients.clear();
		while (!_vce_pclientsBuff.empty())
		{	 
			delete _vce_pclientsBuff.front().get();
			_vce_pclientsBuff.pop();
		}
		return true;
	}

	//移除客户端
	inline void RemoveClient(ClientInfo * pclient)
	{
		if (_INetEvent)
			_INetEvent->OnNetLeave(pclient);
#ifndef _WIN32				 
		epoll.del(pclient->_socket);//EPOLLIN | EPOLLOUT | EPOLLET | EPOLLONESHOT
#endif // !_WIN32

		//_fdRead_change = true;
		--ClientCount;
	}
	//踢出客户端
	void KickClient(ClientInfo * pclient)
	{
#ifdef _WIN32				 
				std::lock_guard<std::mutex> lg(_mutex);
#endif // !_WIN32
		auto del = _vce_pclients.find(pclient->_socket);
		if (del != _vce_pclients.end())
		{
			RemoveClient(del->second.get());
			_vce_pclients.erase(del);
		}
		_fdRead_change = true;
	}

	//检查时间是否超时
	void CheckTime()
	{
#ifdef CLIENT_HREAT_DEAD_TIME
		auto tNow = CELLTime::getNowinMilliSec();
		auto dt   = tNow - _old_time;
		_old_time = tNow;
		for (auto it = _vce_pclients.begin(); it != _vce_pclients.end();)
		{
			//检查是否要踢掉客户端
			if (it->second->checkClient(dt, tNow))
			{
				RemoveClient(it->second.get());		
				it = _vce_pclients.erase(it);
			
				continue;
			}

			//定时发送
			//it->second->checkSend(dt);
			it++;
		}
#endif
	}


#if 0
	//6 接收请求
	int RecvData(ClientInfo* pinfo)
	{	
		//5 接收客户端数据  先接受包头检查数据长度 RECV_BUFF_SIZE
		int rlen = pinfo->RecvData();
		if (rlen < 0)
		{
			//std::cout << "客户端已退出" << pinfo->sock << ", 任务结束!" << std::endl;
			//std::cout << "无数据..." << std::endl;
			return 0;
		}
		else if (rlen == 0)
			return 1;
		//接收事件
		if (_INetEvent)
			_INetEvent->OnNetRevc(pinfo);

		//是否有数据可处理
		while (pinfo->hasMsg())
		{
			//处理
			/*_taskServer.addTask([this, &pinfo]() {
				
			});*/
			OnNetMsg(pinfo);
			pinfo->pop_front();
		}
		return 1;
	}
	//检测是否可写
	void WriteData(fd_set& fdWrite)
	{
		for (size_t i = 0; i < fdWrite.fd_count; i++)
		{
			auto tmp = _vce_pclients.find(fdWrite.fd_array[i]);
			if (_vce_pclients.end() != tmp)
			{
				//可写则发送数据, 如果不可写也发送会造成阻塞
				if (SOCKET_ERROR == tmp->second->SendDateReal())
				{
					Log::Info("###CellServer.WriteData fSock = %d\n", (int)tmp->second->_socket);
					RemoveClient(tmp->second.get());
					_vce_pclients.erase(tmp);
				}
			}
		}
	}
	//检查是否可读
	void ReadData(fd_set& fdRead)
	{
		//执行处理
		for (size_t i = 0; i < fdRead.fd_count; i++)
		{

			auto tmp = _vce_pclients.find(fdRead.fd_array[i]);
			if (_vce_pclients.end() != tmp)
			{
				if (!RecvData(tmp->second.get()))
				{
					RemoveClient(tmp->second.get());
					_vce_pclients.erase(tmp);

				}
			}
		}

	}
#endif


	//7 处理请求
	virtual bool OnNetMsg(ClientInfo* pinfo)
	{
		if (_INetEvent)
			_INetEvent->OnNetMsg(this, pinfo);
		return true;
	}


	//关闭
	void Close()
	{
		_taskServer.Close();
		_thread.Close();		
	}
//启动
	void Start()
	{
		
		_thread.Start([this](CellThread* pthread) {OnRunLoop(pthread);});

		_taskServer.Start();
	}

//添加客户端
	void addClient(ClientInfo* pClient)
	{


#ifdef _WIN32
		std::lock_guard<std::mutex> lg(_mutex);
		if (!iocp.AddSocket(pClient))
			printf("添加socket<%lld>失败\n", pClient->_socket);
		else
			_vce_pclients.insert({ pClient->_socket, std::shared_ptr<ClientInfo>(pClient) });
		++ClientCount;
		if (_INetEvent)
			_INetEvent->OnNetJoin(pClient);
#else		
		if (pClient && ClientCount <= MAX_CLIENT)
		{
			std::lock_guard<std::mutex> lg(_mutex);
			_vce_pclientsBuff.push(std::shared_ptr<ClientInfo>(pClient));
			++ClientCount;
			if (_INetEvent)
				_INetEvent->OnNetJoin(pClient);
		}
#endif
		

	}
	//void addTask(ClientInfo* pClient, DataHeader* pheader)
	//{
	//	_taskServer.addTask([pClient, pheader]() {pClient->SendData(pheader); delete pheader; });
	//}
	int getClientCount()
	{
		return ClientCount;
	}
};
#endif // !CELL_SERVER_HPP
