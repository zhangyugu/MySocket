#ifndef EASY_SELECT_SERVER_HPP
#define EASY_SELECT_SERVER_HPP
#include "GlobalDef.hpp"
#include "EasyTcpServer.hpp"
#include"CellSelectServer.hpp"

class EasySelectServer: public EasyTcpServer
{
private:

public:
	EasySelectServer()
	{	
		
	}
	virtual ~EasySelectServer()
	{
		
	}
	
	void Start(int TCount = 1)
	{
		EasyTcpServer::Start<CellSelectServer>(TCount);
	}

	//等待客户连接
	bool OnRun(CellThread *pthread)
	{
		fd_set _fdRead;
		fd_set _fdWrite;
		fd_set _fdExp;
		while (pthread->isRun())
		{
			calcRecv();
			FD_ZERO(&_fdRead);//清空
			FD_SET(_sock, &_fdRead);//设置
		
			//nfds 是一个整数值, 是指fd_set集合中所有描述符(socket)的范围,
			//而不是数量.既是所有文件描述符中最大值+1, 再windows中这个参数可以写0
			//timeout为NULL时无限等待事件产生
			timeval timeout = { 0,10 };
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


};

#endif