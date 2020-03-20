#pragma once
#ifndef IOCP___HPP
#define IOCP___HPP
// winsock 2 的头文件和库
#include <SockContext.h>
#include <MSWSock.h>
#include <assert.h>
#include <list>
#include <memory>
#include <functional>
#include <vector>
#include <thread>
#include <string>

#include <atomic>
std::atomic<int> count2 = 0;
int count = 0;

#define EXIT_CODE -1
/*char hostname[MAX_PATH] = { 0 };//获取本机名, 可用于获取局域网IP
gethostname(hostname, MAX_PATH);*/
 
enum class REQUEST_TYPE : char
{
	ACCEPT,                     // 标志投递的Accept操作
	SEND,                       // 标志投递的是发送操作
	RECV,                       // 标志投递的是接收操作
	NEW_SEND, 					//
	INVALID                        // 用于初始化，无意义
};

struct Event : public WSAOVERLAPPED
{
	SOCKET _socket{INVALID_SOCKET}, _sockAccept{INVALID_SOCKET};
	sockaddr_in _addr{};
	REQUEST_TYPE type{REQUEST_TYPE::INVALID}/*, type2{ REQUEST_TYPE::INVALID }*/;
	WSABUF         _RecvBuf{}, _SendBuf{};                    // WSA类型的缓冲区，用于给重叠操作传参数的
	unsigned long Bytes{};					 //实际处理的字节数
	Event() :  WSAOVERLAPPED{} {}
	~Event() {}

	virtual void SetRecvWasBuf(){}//设置完成端口接收缓冲区
	virtual void RecvWasBuf(){}//报告实际接收到的字节数, 并解密
	virtual bool isSendWasBuf() { return false; }//是否需要发送数据, 并设置完成端口发送缓冲区
	virtual void ResetSendBuf() {}//清空发送缓存
};

using EventPtr = std::unique_ptr<Event>;
using ACCEPT_BACK = std::function<void(SOCKET &socket, sockaddr_in &addr)>;
using BACK_CALL = std::function<void(void * sockevent)>;
class IOCP
{

private:
	
	HANDLE _hShutdownEvent{};  // 用来通知线程系统退出的事件，为了能够更好的退出线程
	std::vector<std::unique_ptr<std::thread>> _arrThread;
protected:
	HANDLE _PortHandle{};
	char acceptBuf[64]{};
	EventPtr _Listener;
	LPFN_ACCEPTEX  _AcceptEx{};  // AcceptEx 和 GetAcceptExSockaddrs 的函数指针，用于调用这两个扩展函数
	LPFN_GETACCEPTEXSOCKADDRS _GetAcceptExSockAddrs{};
	ACCEPT_BACK _acceptCall{nullptr};
	BACK_CALL _reRecv{ nullptr };
	BACK_CALL _postRecv{ nullptr };//接收前回调, 主要设置缓冲区
	BACK_CALL _DeleteSocket{ nullptr };//移除套接字;


public:
	IOCP() : _Listener{ std::make_unique<Event>() }
	{
		SockContext::Instance();
	}
	virtual ~IOCP()
	{
		Stop();
	}
	
	//创建监听套接字
	bool CreateListenSocket(ACCEPT_BACK acceptCall, unsigned short port, int backlog = 1024)
	{
		Start();
		if (!acceptCall)
			return false;
		_acceptCall = acceptCall;
		// 需要使用重叠IO，必须得使用WSASocket来建立Socket，才可以支持重叠IO操作
		_Listener->_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
		if (_Listener->_socket == INVALID_SOCKET) return 0;


		//绑定监听套接字到监听端口
		if (!CreateIoCompletionPort((HANDLE)_Listener->_socket, _PortHandle, (ULONG_PTR)_Listener.get(), NULL)) return 0;

		//绑定端口
		sockaddr_in _sin = {};
		_sin.sin_family = AF_INET;//地址规范  IPv4 IPv6
		_sin.sin_port = htons(port);//端口   由于不同系统的USHORT字节长度不同, 需要统一转换
		_sin.sin_addr.S_un.S_addr = INADDR_ANY;
		if (SOCKET_ERROR == bind(_Listener->_socket, (sockaddr*)&_sin, sizeof(sockaddr_in)))  return false;

		//监听端口
		if (SOCKET_ERROR == listen(_Listener->_socket, backlog)) return false;

		if (!_AcceptEx)
		{
			//获取acceptEx函数地址
			DWORD dwBytes = 0;
			GUID guidAcceptEx = WSAID_ACCEPTEX;
			if (SOCKET_ERROR == WSAIoctl(_Listener->_socket, SIO_GET_EXTENSION_FUNCTION_POINTER, &guidAcceptEx,
				sizeof(guidAcceptEx), &_AcceptEx, sizeof(_AcceptEx), &dwBytes, nullptr, nullptr))
				throw "获取acceptEx函数地址失败";
		}
		if (!_GetAcceptExSockAddrs)
		{
			DWORD dwBytes = 0;
			// 获取GetAcceptExSockAddrs函数指针，也是同理
			GUID guidGetaddrs = WSAID_GETACCEPTEXSOCKADDRS;
			if (SOCKET_ERROR == WSAIoctl(_Listener->_socket, SIO_GET_EXTENSION_FUNCTION_POINTER, &guidGetaddrs,
				sizeof(guidGetaddrs), &_GetAcceptExSockAddrs, sizeof(_GetAcceptExSockAddrs), &dwBytes, nullptr, nullptr))
				throw "获取GetAcceptExSockAddrs函数地址失败";
		}

		_Listener->type = REQUEST_TYPE::ACCEPT;

		return PostAccept();
	}
	void Start(int threadsum = 0)
	{
		if (!_PortHandle)
		{
			// 建立监听完成端口
			if (!(_PortHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, NULL)))
				throw "建立监听完成端口失败";

			// 建立系统退出的事件通知
			_hShutdownEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

			if (0 == threadsum)
				threadsum = std::thread::hardware_concurrency();
			//启动线程
			for (unsigned int i = 0; i < threadsum; ++i)
			{
				_arrThread.emplace_back(std::make_unique<std::thread>([this]() {Run(); }));
			}
		}	
	}
	void Stop()//开始发送系统退出消息，退出完成端口和线程资源
	{
		if (_Listener.get() && _Listener->_socket != INVALID_SOCKET)
		{
			// 激活关闭消息通知
			SetEvent(_hShutdownEvent);

			for (auto &x : _arrThread)
			{
				PostQueuedCompletionStatus(_PortHandle, 0, (DWORD)EXIT_CODE, NULL);
			}

			for (auto &x : _arrThread)
			{
				if (x->joinable())
					x->join();
			}
			_arrThread.clear();

			if (_hShutdownEvent && _hShutdownEvent != INVALID_HANDLE_VALUE)
			{
				CloseHandle(_hShutdownEvent);
				_hShutdownEvent = nullptr;
			}
			if (_PortHandle && _PortHandle != INVALID_HANDLE_VALUE)
			{
				CloseHandle(_PortHandle);
				_PortHandle = nullptr;
			}

			closesocket(_Listener->_socket);
		}
	}

	//添加到端口
	bool AddSocket(Event *sockevent)// 添加套接字
	{
		if (!CreateIoCompletionPort((HANDLE)sockevent->_socket, _PortHandle, (ULONG_PTR)sockevent, NULL))
			return false;
		//_arrEvent.emplace_back(ev);
		return PostRecv(sockevent);
	}
	
	//发送接受连接请求
	bool PostAccept()
	{
		assert(INVALID_SOCKET != _Listener->_socket);

		// 准备参数
		DWORD dwBytes = 0;

		// 为以后新连入的客户端先准备好Socket( 这个是与传统accept最大的区别 ) 
		_Listener->_sockAccept = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
		if (INVALID_SOCKET == _Listener->_sockAccept)
		{
			printf("创建用于Accept的Socket失败！错误代码: %d\n", WSAGetLastError());
			return false;
		}

		// 投递AcceptEx
		if (FALSE == _AcceptEx(
			_Listener->_socket,
			_Listener->_sockAccept,
			acceptBuf,
			0,
			sizeof(SOCKADDR_IN) + 16,
			sizeof(SOCKADDR_IN) + 16,
			&dwBytes,
			_Listener.get()))
		{
			if (WSA_IO_PENDING != WSAGetLastError())
			{
				printf("投递 AcceptEx 请求失败，错误代码: %d\n", WSAGetLastError());
				return false;
			}
		}
		return true;
	}


	//发送接收数据请求
	bool PostRecv(Event *sockevent)
	{
		DWORD dwFlags = 0, Bytes = 0;
		//sockevent->ResetBuffer();
		sockevent->SetRecvWasBuf();//设置完成端口接收缓冲区	
		if (_postRecv)
			_postRecv(sockevent);
		//printf("count<%d>RecvBuf<%p>, Size<%d>\n", ++count, sockevent->_Buf.buf, sockevent->_Buf.len);
		sockevent->type = REQUEST_TYPE::RECV;
		int nBytesRecv = WSARecv(sockevent->_socket, &sockevent->_RecvBuf, 1, &Bytes, &dwFlags, sockevent, NULL);
		
		// 如果返回值错误，并且错误的代码并非是Pending的话，那就说明这个重叠请求失败了
		if ((SOCKET_ERROR == nBytesRecv) && (WSA_IO_PENDING != (nBytesRecv = WSAGetLastError())))//
		{
			if (WSAEFAULT == nBytesRecv)
				printf("投递一个WSARecv失败 系统在尝试使用调用的指针参数时检测到无效的指针地址\n");
			else
				printf("投递第一个WSARecv失败 Error: %d\n", nBytesRecv);
			return false;
		}
		return true;
	}
	
	//发送发送数据请求
	bool PostSend(Event *sockevent)
	{
		DWORD dwFlags = 0;
		DWORD dwBytes = 0;		
		sockevent->type = REQUEST_TYPE::SEND;
		int nBytesRecv = WSASend(sockevent->_socket, &sockevent->_SendBuf, 1, &dwBytes, dwFlags, sockevent, NULL);
		if ((SOCKET_ERROR == nBytesRecv) && (WSA_IO_PENDING != WSAGetLastError()))
		{// 如果返回值错误，并且错误的代码并非是Pending的话，那就说明这个重叠请求失败了
			printf("投递第一个WSASend失败: %d\n", WSAGetLastError());
			return false;
		}
		return true;
	}

	bool newPostSend(SOCKET socket, char * buf, unsigned int buflen)
	{
		DWORD dwFlags = 0;
		DWORD dwBytes = 0;
		Event * pEvent = new Event;
		pEvent->_socket = socket;
		pEvent->type = REQUEST_TYPE::NEW_SEND;
		pEvent->_SendBuf.buf = new char[buflen];
		pEvent->_SendBuf.len = buflen;
		memcpy(pEvent->_SendBuf.buf, buf, buflen);

		int nBytesRecv = WSASend(pEvent->_socket, &pEvent->_SendBuf, 1, &dwBytes, dwFlags, pEvent, NULL);
		if ((SOCKET_ERROR == nBytesRecv) && (WSA_IO_PENDING != WSAGetLastError()))
		{// 如果返回值错误，并且错误的代码并非是Pending的话，那就说明这个重叠请求失败了
			printf("投递第一个WSASend失败: %d\n", WSAGetLastError());
			return false;
		}
		return true;
	}
	
	//设置事件回调
	void SetDeleteSocket(BACK_CALL deletesocket) { _DeleteSocket = deletesocket; }
	void SetRecvBackCall(BACK_CALL porecv, BACK_CALL dorecv) {
		_reRecv = dorecv; _postRecv  = porecv;
	};

protected:
	virtual bool DoAccpet()//请求完成
	{
		sockaddr_in* ClientAddr = NULL;
		sockaddr_in* LocalAddr = NULL;
		int remoteLen = sizeof(SOCKADDR_IN), localLen = sizeof(SOCKADDR_IN);


		// 取得客户端和本地端的地址信息，可以顺便取出客户端发来的第一组数据
		_GetAcceptExSockAddrs(acceptBuf, 0/*pIoContext->m_wsaBuf.len - ((sizeof(SOCKADDR_IN) + 16) * 2)*/,
			sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, (LPSOCKADDR*)&LocalAddr, &localLen, (LPSOCKADDR*)&ClientAddr, &remoteLen);

		if (_acceptCall)
			_acceptCall(_Listener->_sockAccept, *ClientAddr);

		return PostAccept();
	}
	virtual void DoRecv(Event *pEvent)
	{
		pEvent->RecvWasBuf();//更新接收缓冲区
		if (_reRecv)
			_reRecv(pEvent);
		if (pEvent->isSendWasBuf())//是否需要发送数据, 需要就设置完成端口发送缓冲区
		{
			if (!PostSend(pEvent))
				_DeleteSocket(pEvent);
		}
		else
		{
			//投递下一个WSARecv请求
			if (!PostRecv(pEvent))
				_DeleteSocket(pEvent);
		}
	}
	virtual void DoSend(Event *pEvent)
	{
		pEvent->ResetSendBuf();
		if (!PostRecv(pEvent))
			_DeleteSocket(pEvent);
	}
private:
	//线程运行
	void Run()
	{
		OVERLAPPED           *pOverlapped = NULL;
		DWORD                dwBytesTransfered = 0;
		ULONG_PTR			 KeyValue;//注册是或者PostQueuedCompletionStatus发送过来的键值

		int thread = ++count2;
		// 循环处理请求，知道接收到Shutdown信息为止
		while (WAIT_OBJECT_0 != WaitForSingleObject(_hShutdownEvent, 0))
		{
			BOOL bReturn = GetQueuedCompletionStatus(
				_PortHandle,
				&dwBytesTransfered,
				&KeyValue,
				&pOverlapped,
				INFINITE);

			
			// 如果收到的是退出标志，则直接退出
			if (EXIT_CODE == KeyValue)
			{
				break;
			}
			//Event* pEvent = (Event*)KeyValue;
			Event * pEvent = static_cast<Event*>(pOverlapped);
			// 判断是否出现了错误
			if (!bReturn)
			{
				DWORD dwErr = GetLastError();
				// 显示一下提示信息
				// 如果是超时了，就再继续等吧  
				if (WAIT_TIMEOUT == dwErr)
				{
					// 确认客户端是否还活着...
					if (-1 == send(pEvent->_socket, "", 0, 0))
					{
						//printf("超时, 检测到客户端异常退出！\n");
						_DeleteSocket(pEvent);
					}
				}
				// 可能是客户端异常退出了
				else if (ERROR_NETNAME_DELETED == dwErr)
				{
					//printf("检测到客户端异常退出！\n");
					_DeleteSocket(pEvent);
				}
				else if (ERROR_CONNECTION_ABORTED == dwErr)
				{
					//printf("网络连接被本地系统中止\n");
					_DeleteSocket(pEvent);
				}
				else
				{
					//printf("完成端口操作出现错误，线程退出。错误代码：%d\n", dwErr);		
					printf("完成端口操作出现未知错误。错误代码：%d\n", dwErr);
				}

				continue;
			}
			else
			{
				// 读取传入的参数
				//Event* pIoContext = CONTAINING_RECORD(pOverlapped, Event, OVERLAPPED);
				pEvent->Bytes = dwBytesTransfered;		
				if ((0 == dwBytesTransfered) && (REQUEST_TYPE::RECV == pEvent->type || REQUEST_TYPE::SEND == pEvent->type))
				{// 判断是否有客户端断开了
					_DeleteSocket(pEvent);					
					continue;
				}
				else
				{
					switch (pEvent->type)
					{
					case REQUEST_TYPE::ACCEPT:
					{
						DoAccpet();
					}
					break;
					case REQUEST_TYPE::RECV:
					{	
						
						DoRecv(pEvent);
						
					}
					break;
					case REQUEST_TYPE::SEND:
					{				
						DoSend(pEvent);					
					}
					break;
					case REQUEST_TYPE::NEW_SEND:
					{
						delete pEvent->_SendBuf.buf;
						delete pEvent;
					}
					break;
					default:
						printf("IOCP未知请求类型.\n");
						break;
					}
					
		
				}//if
			}//if

		}//while

	}
};
#endif


