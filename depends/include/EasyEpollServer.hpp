#ifndef EASY_SELECT_SERVER_HPP
#define EASY_SELECT_SERVER_HPP
#include "GlobalDef.hpp"
#include "EasyTcpServer.hpp"
#include"CellEpollServer.hpp"

#if __linux__
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
		EasyTcpServer::Start<CellEpollServer>(TCount);
	}

	//等待客户连接
	bool OnRun(CellThread *pthread)
	{
		Epoll ep;
		ep.add(_sock, &_sock, EPOLLIN);
		while (pthread->isRun())
		{
			calcRecv();

			int ret = ep.wait(1);
			if (ret < 0)
			{
				Log::Info("TcpServer.OnRunLoop.select Error!\n");
				break;
			}

			else if (ret == 0)
				continue;
			
			if (events[i].events & EPOLLIN)
			{
				Accept();
			}
				
			
		}
		
		

		return false;
	}


};
#endif //__linux__
#endif