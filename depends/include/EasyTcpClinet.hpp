#ifndef EASY_TCP_CLIENT_HPP
#define EASY_TCP_CLIENT_HPP
#include "SockContext.h"
#include "GlobalDef.hpp"
#include "ClientInfo.hpp"
#include "IOCP.hpp"

#include <Log.hpp>
std::atomic_int sendCount = 0;//发送计数

class EasyTcpClient : public IOCP
{
protected:
	//处理消息
	virtual void OnNetMsg()
	{
		switch (pClinet->front()->cmd)
		{
		case CMD::HEART_S:
		{
			//char now[64]{};
			//CELLTime::getSystemTime(now);
			//printf("收到心跳包!  %s\n\n", now);
			_dtHeart = 0;
			_bHeart = true;
		/*	DataHeader h;
			h.cmd = CMD::HEART_C;
			h.datalength = 4;
			pClinet->SendData(&h);*/
		}
		break;
		}
	}
protected:
	ClientInfo * pClinet;
	const char *_Heart = "\x4\x0\x6\x0";//心跳包
private:
	sockaddr_in _sin;//地址  IP、IP类型、端口
	fd_set _fdRead, _fdWrite;
	time_t _old_time, _dtHeart;//心跳检测
	CellThread _therad;//主线程
	bool _bHeart;
public:
	EasyTcpClient() :_dtHeart(0), _bHeart(true), pClinet(nullptr)
	{		
		_old_time = CELLTime::getNowinMilliSec();
		IOCP::Start(2);
	}
	virtual ~EasyTcpClient()
	{
		Close();		
	}

	//初始化socket 1 建立一个 socket
	int initSocket()
	{
		SockContext::Instance();
		
		if (pClinet)//已建立则关闭
		{
			Log::Info("warning, initSocket close old socket<%d>...\n", (int)pClinet->_socket);
			pClinet->CloseSock();
		}

		SOCKET sock = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED); //socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (INVALID_SOCKET == sock)
		{
			Log::Info("error, create socket failed...\n" );
			return -1;
		}
		else
		{		

			pClinet = new ClientInfo(sock);
			if (!CreateIoCompletionPort((HANDLE)pClinet->_socket, _PortHandle, (ULONG_PTR)pClinet, NULL))
			{
				delete pClinet;
				return false;
			}			
		}
		return 0;
	}

	//2 连接服务器
	bool Connect(const char* ip, unsigned short port)
	{
		
		//1 建立一个 socket
		if (!pClinet)//没运行就初始化
		{
			initSocket();
		}

		//std::cout << "<socket: " << _sock << " >尝试连接 " << ip << " : " << port << std::endl;
		// 2 连接服务器 connect
		_sin.sin_family = AF_INET;
		_sin.sin_port = htons(port);
#ifdef _WIN32
		//_sin.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
		_sin.sin_addr.S_un.S_addr = inet_addr(ip);
#else
		_sin.sin_addr.s_addr = inet_addr(ip);
#endif
		
		if (SOCKET_ERROR == connect(pClinet->_socket, (sockaddr*)&_sin, sizeof(sockaddr_in)))
		{
			Log::Info("error, socket<%d> connect Ip<%s: %d> failed...\n", pClinet->_socket, ip, port);
			Close();
			return false;
		}
		//printf("已连接到<%s : %d>\n", ip, port);		
		_therad.Start([this](CellThread *pthread) {OnRunLoop(pthread); });
		
		return true;
	}

	//等待消息
	bool OnRunLoop(CellThread *pthread)
	{
		SetDeleteSocket([this](void * client) {
			//与服务器断开连接
			Log::Info("socket<%d> Network connection interruption exit...\n", (int)pClinet->_socket);
			Close();
		});
		SetRecvBackCall(nullptr, [this](void* client) {
			ClientInfo* pclient = static_cast<ClientInfo*>(client);
			while (pclient->hasMsg())
			{
				//处理
				OnNetMsg();
				pclient->pop_front();
			}		
		});


			
		pClinet->SendData((char*)_Heart, 4);
		
		pClinet->isSendWasBuf();
		PostSend(pClinet);

		
		while (pClinet && pthread->isRun())
		{
			//FD_ZERO(&_fdRead);//清空
			//FD_SET(pClinet->_socket, &_fdRead);//设置
			//timeval timeout = { 0,1 };
			//int ret = 0;
			//if (pClinet->needWrite())//检查发送缓冲区是否有数据
			//{
			//	FD_ZERO(&_fdWrite);//清空
			//	FD_SET(pClinet->_socket, &_fdWrite);//设置
			//	ret = select((int)pClinet->_socket + 1, &_fdRead, &_fdWrite, nullptr, &timeout);
			//}
			//else
			//{
			//	ret = select((int)pClinet->_socket + 1, &_fdRead, nullptr, nullptr, &timeout);
			//}
			//if (ret < 0)
			//{
			//	Log::Info("socket<%d> OnRunLoop.select exit...\n", (int)pClinet->_socket);
			//	return true;
			//}
			//if (FD_ISSET(pClinet->_socket, &_fdRead))//检查
			//{			
			//	FD_CLR(pClinet->_socket, &_fdRead);//删除		
			//	//执行处理
			//	
			//	if (!RecvData())
			//	{
			//		//与服务器断开连接
			//		Log::Info("socket<%d> Network connection interruption exit...\n", (int)pClinet->_socket);
			//		Close();
			//		return true;
			//	}
			//		
			//}		
			//if (FD_ISSET(pClinet->_socket, &_fdWrite))//检查
			//{
			//	FD_CLR(pClinet->_socket, &_fdWrite);//删除		
			//	//执行处理 发送数据
			//	if (SOCKET_ERROR == pClinet->SendDateReal())
			//	{
			//		//与服务器断开连接
			//		Close();
			//		return true;
			//	}
			//	/*else
			//	{
			//		char now[64]{};
			//		CELLTime::getSystemTime(now);
			//		printf("发送成功!  %s\n", now);
			//	}*/
			//}

#ifdef CLIENT_HREAT_DEAD_TIME
			auto t = CELLTime::getNowinMilliSec();
			_dtHeart += t - _old_time;
			_old_time = t;

			if (_dtHeart >= CLIENT_HREAT_DEAD_TIME)
			{
				Log::Info("OnRunLoop.heart timeout: %d\n", (int)_dtHeart);
				Close();
				return true;
			}
			else if (_bHeart && _dtHeart >= CLIENT_HREAT_DEAD_TIME * 0.8f)
			{
				//printf("心跳: %d\n", (int)_dtHeart);
				_bHeart = false;
#ifdef _BASE64

				std::string str = pClinet->Base64(std::string(_Heart, 4));
				newPostSend(pClinet->_socket, (char*)str.c_str(), str.size());
#else
				newPostSend(pClinet->_socket, (char*)_Heart, 4);			
#endif
				
			}
#endif // CLIENT_HREAT_DEAD_TIME
			CellThread::sleep(1000);
			
		}
		
			return true;	
	}

	//接收数据  处理粘包、分包
	int RecvData()
	{
		//5 接收客户端数据  先接受包头检查数据长度 RECV_BUFF_SIZE
		int rlen = pClinet->RecvData();
		if (rlen < 0)
		{
			//std::cout << "客户端已退出" << pinfo->sock << ", 任务结束!" << std::endl;
			//std::cout << "无数据..." << std::endl;
			return 0;
		}
		else if (rlen == 0)
			return 1;

	

		//是否有数据可处理
		while (pClinet->hasMsg())
		{
			//处理
			OnNetMsg();
			pClinet->pop_front();
		}
		return 1;
	}
	
	//发送数据
	int SendData(char *header, int length)
	{	
		
		if (pClinet && header)
		{
			int len = pClinet->SendData(header, length);
			if (len != SOCKET_ERROR)
				sendCount++;
			return len;
		}	
		else
			return -1;
	}
	//发送数据
	int SendData(DataHeader * header)
	{

		if (pClinet && header)
		{
			int len = pClinet->SendData(header);
			if (len != SOCKET_ERROR)
				sendCount++;
			return len;
		}
		else
			return -1;
	}

	int Send(DataHeader * header)
	{
		return pClinet->Send(header);
	}

	//关闭socket
	void Close()
	{
		IOCP::Stop();
		_therad.Close();
		if (pClinet)
		{ 
			delete pClinet;
			pClinet = nullptr;
		}
	}

private:
	

};


#endif

