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
	std::vector<std::shared_ptr<ClientInfo>> _vce_pclientsBuff;//缓冲客户队列
	std::mutex _mutex;//缓冲区线程锁
	INetEvent *_INetEvent;//事件
	CellTaskServer _taskServer;
	time_t _old_time;
	CellThread _thread;//线程
protected:
	bool _fdRead_change;//客户端数量是否有改变
	std::map<SOCKET, std::shared_ptr<ClientInfo>> _vce_pclients;//客户队列
protected:
	virtual bool doNetEvent() = 0;
public:
	explicit CellServer(INetEvent *eventObj = nullptr) :
		_INetEvent(eventObj), ClientCount(0), _fdRead_change(true)
	{

	}
	virtual ~CellServer()
	{
		Close();
	}

	CellServer(const CellServer&) = delete;
	CellServer& operator=(const CellServer&) = delete;

	bool OnRun(CellThread *thread)
	{
		_old_time = CELLTime::getNowinMilliSec();
		while (thread->isRun())
		{

			if (!_vce_pclientsBuff.empty())
			{
				//从缓冲区取出客户数据
				std::lock_guard<std::mutex> lg(_mutex);
				for (auto &x : _vce_pclientsBuff)
				{
					_vce_pclients.insert({ x->_socket, x });
					++ClientCount;
					if (_INetEvent)
						_INetEvent->OnNetJoin(x.get());
					OnclientJoin(x.get());
				}
					

				_vce_pclientsBuff.clear();
				_fdRead_change = true;
			}

			//没有客户端
			if (_vce_pclients.empty())
			{
				//休眠 1 毫秒			
				std::chrono::milliseconds t(1);
				std::this_thread::sleep_for(t);
				_old_time = CELLTime::getNowinMilliSec();
				continue;
			}

			CheckTime();
			if (!doNetEvent())
				break;
			
			doMsg();
		}

		//关闭所有客户端的套接字
		/*
			stl容器的clear() erase() 会调用元素的析构函数
			shared_ptr析构会调用delete 释放其保存的指针
			delete会调用ClientInfo的析构关闭sock
		*/
		_vce_pclients.clear();
		_vce_pclientsBuff.clear();
		return true;
	}
	void doMsg()
	{
		for (auto &client : _vce_pclients)
		{
			//是否有数据可处理
			while (client.second->hasMsg())
			{
				//处理
				OnNetMsg(client.second.get());
				client.second->pop_front();
			}
		}
		
	}
	bool doSelect()
	{
		fd_set fdRead, fdRead_back, fdWrite, fdExp;
		SOCKET Max = 0;
		if (_fdRead_change)
		{
			_fdRead_change = false;
			FD_ZERO(&fdRead);//清空
			FD_ZERO(&fdWrite);
			FD_ZERO(&fdExp);
			//将所有客户机添加到集合
			Max = _vce_pclients.begin()->first;//最大值， 用于 select
			for (auto x : _vce_pclients)
			{
				FD_SET(x.first, &fdRead);
				if (Max < x.first)
					Max = x.first;
			}
			memcpy(&fdRead_back, &fdRead, sizeof(fd_set));
		}
		else
			memcpy(&fdRead, &fdRead_back, sizeof(fd_set));
		memcpy(&fdWrite, &fdRead_back, sizeof(fd_set));
		memcpy(&fdExp, &fdRead_back, sizeof(fd_set));

		//nfds 是一个整数值, 是指fd_set集合中所有描述符(socket)的范围,
		//而不是数量.既是所有文件描述符中最大值+1, 再windows中这个参数可以写0
		//timeout为NULL时无限等待事件产生
		timeval t{ 0, 1 };
		if (select((int)Max + 1, &fdRead, &fdWrite, &fdExp, &t) < 0)
		{
			Log::Info("CellServer.OnRunLoop.select Error!\n");
			return false;
		}
		//执行处理

		//fdRead.fd_array + FD_SETSIZE != std::find(fdRead.fd_array, fdRead.fd_array + FD_SETSIZE, (*it)->sock)
		ReadData(fdRead);
		WriteData(fdWrite);
		//WriteData(fdExp);
#ifdef _WIN32
		if (fdExp.fd_count > 0)
		{
			Log::Info("###CellServer.OnRunLoop fdExp = %d\n", fdExp.fd_count);
		}
#endif
	}
	//移除客户端
	
	inline void RemoveClient(ClientInfo * pclient)
	{
		if (_INetEvent)
			_INetEvent->OnNetLeave(pclient);
		_fdRead_change = true;
		--ClientCount;
	}
	
	virtual void OnclientJoin(ClientInfo * pclient)
	{

	}
	void CheckTime()
	{
		auto tNow = CELLTime::getNowinMilliSec();
		auto dt   = tNow - _old_time;
		_old_time = tNow;
		for (auto it = _vce_pclients.begin(); it != _vce_pclients.end();)
		{
			//心跳检测
			if (it->second->checkHeart(dt))
			{
				RemoveClient(it->second.get());
				it = _vce_pclients.erase(it);
			
				continue;
			}

			//定时发送
			//it->second->checkSend(dt);
			it++;
		}
	}

	//检测是否可写
	void WriteData(fd_set& fdWrite)
	{
		
#ifdef _WIN32

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

#else
		auto it = _vce_pclients.begin();
		while (it != _vce_pclients.end())
		{

			if (FD_ISSET(it->second->sock, &fdWrite))
			{
				if (!RecvData(it->second.get()))
				{
					RemoveClient(it->second.get());
					it = _vce_pclients.erase(it);//在容器循环中删除其元素
					continue;
				}
			}
			++it;
		}

#endif // !_WIN32
	}
	void ReadData(fd_set& fdRead)
	{
		//执行处理
#ifdef _WIN32

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
		
#else
		auto it = _vce_pclients.begin();
		while (it != _vce_pclients.end())
		{

			if (FD_ISSET(it->second->sock, &fdRead))
			{
				if (!RecvData(it->second.get()))
				{
					RemoveClient(it->second.get());
					it = _vce_pclients.erase(it);//在容器循环中删除其元素
					continue;
				}
			}
			++it;
		}

#endif // !_WIN32
	}

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

		
		return rlen;
	}

	//7 处理请求
	virtual bool OnNetMsg(ClientInfo* pinfo)
	{
		if (_INetEvent)
			_INetEvent->OnNetMsg(this, pinfo);
		return true;
	}


	//9 关闭
	void Close()
	{

		_taskServer.Close();
		_thread.Close();
		

	}

//	inline void CloseSock(SOCKET sock)
//	{
//		if (INVALID_SOCKET != sock)
//		{
//#ifdef _WIN32
//			closesocket(sock);
//#else
//			close(sock);
//#endif
//			sock = INVALID_SOCKET;
//		}
//	}

	void Start()
	{
		
		_thread.Start([this](CellThread* pthread) {OnRun(pthread);});

		_taskServer.Start();
	}


	void addClient(ClientInfo* pClient)
	{
		if (pClient && ClientCount <= FD_SETSIZE)
		{
			std::lock_guard<std::mutex> lg(_mutex);
			_vce_pclientsBuff.push_back(std::shared_ptr<ClientInfo>(pClient));
		}

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