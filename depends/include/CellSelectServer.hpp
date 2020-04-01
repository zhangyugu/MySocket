#ifndef CELL_SELECT_SERVER_HPP
#define CELL_SELECT_SERVER_HPP
#include"GlobalDef.hpp"
#include "CellServer.hpp"


class CellSelectServer : public CellServer
{

public:

	CellSelectServer(INetEvent *eventObj = nullptr) : CellServer(eventObj) {}
	CellSelectServer(const CellSelectServer&) = delete;
	CellSelectServer& operator= (const CellSelectServer&) = delete;
	bool doNetEvent()
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
		return true;
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



};

#endif // !CELL_SERVER_HPP