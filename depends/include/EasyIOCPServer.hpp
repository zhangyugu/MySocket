#ifndef EASY_IOCP_SERVER_HPP
#define EASY_IOCP_SERVER_HPP
#include "EasyTcpServer.hpp"
#include "GlobalDef.hpp"
#include"CellServer.hpp"
#include "SockContext.h"
#include "IOCP.hpp"

class EasyIOCPServer: public EasyTcpServer
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
public:
	void Start(int nServer)
	{
		EasyTcpServer::Start()
	}
protected:
	//等待客户连接
	bool OnRunLoop(CellThread *pthread)
	{
		IOCP iocp;
		iocp.create();
		iocp.reg(_sock);
		iocp.loadFunction(_sock);
		IO_DATA_BASE ioData = {};
		IO_EVENT ioEvent = {};

		iocp.postAccept(&ioData);
		while (pthread->isRun())
		{
			calcRecv();

			int ret = iocp.wait(ioEvent, 1);
			if (ret < 0)
			{
				Log::Info("TcpServer.OnRunLoop.select Error!\n");
				break;
			}
			else if (ret == 0)
				continue;
			if (IO_TYPE::ACCEPT == ioEvent.pIoData->type)
			{
				iocp.postAccept(&ioData);
			}
		}
		
		return false;
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