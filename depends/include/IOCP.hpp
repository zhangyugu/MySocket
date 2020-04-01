#pragma once
#ifndef IOCP___HPP
#define IOCP___HPP
#ifdef _WIN32
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
/*char hostname[MAX_PATH] = { 0 };//��ȡ������, �����ڻ�ȡ������IP
gethostname(hostname, MAX_PATH);*/
 
enum class REQUEST_TYPE : char
{
	ACCEPT,                     // ��־Ͷ�ݵ�Accept����
	SEND,                       // ��־Ͷ�ݵ��Ƿ��Ͳ���
	RECV,                       // ��־Ͷ�ݵ��ǽ��ղ���
	NEW_SEND, 					//
	INVALID                        // ���ڳ�ʼ����������
};

enum class IO_TYPE : char {
	ACCEPT,
	RECR,
	SEND,
	INVALID
};

#define IO_DATA_BUFF_SIZE 1024
struct IO_DATA_BASE
{
	//�ص���
	WSAOVERLAPPED overlapped;
	SOCKET sockfd;
	//���ݻ�����
	char buffer[IO_DATA_BUFF_SIZE];
	int length;
	//��������
	IO_TYPE type;
};

struct IO_EVENT 
{
	IO_DATA_BASE* pIoData;
	SOCKET socket = INVALID_SOCKET;
	DWORD btesTrans = 0;
};

struct Event : public WSAOVERLAPPED
{
	SOCKET _socket{INVALID_SOCKET}, _sockAccept{INVALID_SOCKET};
	sockaddr_in _addr{};
	REQUEST_TYPE type{REQUEST_TYPE::INVALID}/*, type2{ REQUEST_TYPE::INVALID }*/;
	WSABUF         _RecvBuf{}, _SendBuf{};                    // WSA���͵Ļ����������ڸ��ص�������������
	unsigned long Bytes{};					 //ʵ�ʴ�����ֽ���
	Event() :  WSAOVERLAPPED{} {}
	~Event() {}

	virtual void SetRecvWasBuf(){}//������ɶ˿ڽ��ջ�����
	virtual void RecvWasBuf(){}//����ʵ�ʽ��յ����ֽ���, ������
	virtual bool isSendWasBuf() { return false; }//�Ƿ���Ҫ��������, ��������ɶ˿ڷ��ͻ�����
	virtual void ResetSendBuf() {}//��շ��ͻ���
};

using EventPtr = std::unique_ptr<Event>;
using ACCEPT_BACK = std::function<void(SOCKET &socket, sockaddr_in &addr)>;
using BACK_CALL = std::function<void(void * sockevent)>;
class IOCP
{

private:
	
	HANDLE _hShutdownEvent{};  // ����֪ͨ�߳�ϵͳ�˳����¼���Ϊ���ܹ����õ��˳��߳�
	std::vector<std::unique_ptr<std::thread>> _arrThread;
protected:
	HANDLE _PortHandle{};
	char acceptBuf[1024]{};
	EventPtr _Listener;
	SOCKET _ListenSocket  = INVALID_SOCKET;
	LPFN_ACCEPTEX  _AcceptEx{};  // AcceptEx �� GetAcceptExSockaddrs �ĺ���ָ�룬���ڵ�����������չ����
	LPFN_GETACCEPTEXSOCKADDRS _GetAcceptExSockAddrs{};
	ACCEPT_BACK _acceptCall{nullptr};
	BACK_CALL _reRecv{ nullptr };
	BACK_CALL _postRecv{ nullptr };//����ǰ�ص�, ��Ҫ���û�����
	BACK_CALL _DeleteSocket{ nullptr };//�Ƴ��׽���;


public:
	IOCP() : _Listener{ std::make_unique<Event>() }
	{
		SockContext::Instance();
	}
	virtual ~IOCP()
	{
		Stop();
	}
	
	//���������׽���
	bool CreateListenSocket(ACCEPT_BACK acceptCall, unsigned short port, int backlog = 1024)
	{
		Start();
		if (!acceptCall)
			return false;
		_acceptCall = acceptCall;
		// ��Ҫʹ���ص�IO�������ʹ��WSASocket������Socket���ſ���֧���ص�IO����
		_Listener->_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (_Listener->_socket == INVALID_SOCKET) return 0;


		//�󶨼����׽��ֵ������˿�
		if (!CreateIoCompletionPort((HANDLE)_Listener->_socket, _PortHandle, (ULONG_PTR)_Listener.get(), NULL)) return 0;

		//�󶨶˿�
		sockaddr_in _sin = {};
		_sin.sin_family = AF_INET;//��ַ�淶  IPv4 IPv6
		_sin.sin_port = htons(port);//�˿�   ���ڲ�ͬϵͳ��USHORT�ֽڳ��Ȳ�ͬ, ��Ҫͳһת��
		_sin.sin_addr.S_un.S_addr = INADDR_ANY;
		if (SOCKET_ERROR == bind(_Listener->_socket, (sockaddr*)&_sin, sizeof(sockaddr_in)))  return false;

		//�����˿�
		if (SOCKET_ERROR == listen(_Listener->_socket, backlog)) return false;

		if (!_AcceptEx)
		{
			//��ȡacceptEx������ַ
			DWORD dwBytes = 0;
			GUID guidAcceptEx = WSAID_ACCEPTEX;
			if (SOCKET_ERROR == WSAIoctl(_Listener->_socket, SIO_GET_EXTENSION_FUNCTION_POINTER, &guidAcceptEx,
				sizeof(guidAcceptEx), &_AcceptEx, sizeof(_AcceptEx), &dwBytes, nullptr, nullptr))
				throw "��ȡacceptEx������ַʧ��";
		}
		if (!_GetAcceptExSockAddrs)
		{
			DWORD dwBytes = 0;
			// ��ȡGetAcceptExSockAddrs����ָ�룬Ҳ��ͬ��
			GUID guidGetaddrs = WSAID_GETACCEPTEXSOCKADDRS;
			if (SOCKET_ERROR == WSAIoctl(_Listener->_socket, SIO_GET_EXTENSION_FUNCTION_POINTER, &guidGetaddrs,
				sizeof(guidGetaddrs), &_GetAcceptExSockAddrs, sizeof(_GetAcceptExSockAddrs), &dwBytes, nullptr, nullptr))
				throw "��ȡGetAcceptExSockAddrs������ַʧ��";
		}

		_Listener->type = REQUEST_TYPE::ACCEPT;

		return PostAccept();
	}
	void Start(int threadsum = 0)
	{
		if (!_PortHandle)
		{
			// ����������ɶ˿�
			if (!(_PortHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, NULL)))
				throw "����������ɶ˿�ʧ��";

			// ����ϵͳ�˳����¼�֪ͨ
			_hShutdownEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

			if (0 == threadsum)
				threadsum = std::thread::hardware_concurrency();
			//�����߳�
			for (unsigned int i = 0; i < threadsum; ++i)
			{
				_arrThread.emplace_back(std::make_unique<std::thread>([this]() {Run(); }));
			}
		}	
	}
	void Stop()//��ʼ����ϵͳ�˳���Ϣ���˳���ɶ˿ں��߳���Դ
	{
		if (_Listener.get() && _Listener->_socket != INVALID_SOCKET)
		{
			// ����ر���Ϣ֪ͨ
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

	bool create()
	{
		if (!_PortHandle)
		{
			// ����������ɶ˿�
			if (!(_PortHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, NULL)))
				return false;
		}
		return true;
	}

	void destory()
	{
		if (_PortHandle)
		{
			CloseHandle(_PortHandle);
			_PortHandle = nullptr;
		}		
	}

	bool reg(SOCKET sockfd)
	{
		if (!CreateIoCompletionPort((HANDLE)sockfd, _PortHandle, (ULONG_PTR)sockfd, NULL))
		{
			printf("socket<%d>������ɶ˿�ʧ��, ERROR: %d\n", sockfd, GetLastError());
			return false;
		}
		return true;
	}

	void loadFunction(SOCKET listenSocket)
	{
		if (_ListenSocket != INVALID_SOCKET)
		{	
			printf("ERROR: loaded\n");
			return;
		}
		_Listener->_socket = listenSocket;

		if (!_AcceptEx)
		{
			//��ȡacceptEx������ַ
			DWORD dwBytes = 0;
			GUID guidAcceptEx = WSAID_ACCEPTEX;
			if (SOCKET_ERROR == WSAIoctl(_ListenSocket, SIO_GET_EXTENSION_FUNCTION_POINTER, &guidAcceptEx,
				sizeof(guidAcceptEx), &_AcceptEx, sizeof(_AcceptEx), &dwBytes, nullptr, nullptr))
				throw "��ȡacceptEx������ַʧ��";
		}
		if (!_GetAcceptExSockAddrs)
		{
			DWORD dwBytes = 0;
			// ��ȡGetAcceptExSockAddrs����ָ�룬Ҳ��ͬ��
			GUID guidGetaddrs = WSAID_GETACCEPTEXSOCKADDRS;
			if (SOCKET_ERROR == WSAIoctl(_ListenSocket, SIO_GET_EXTENSION_FUNCTION_POINTER, &guidGetaddrs,
				sizeof(guidGetaddrs), &_GetAcceptExSockAddrs, sizeof(_GetAcceptExSockAddrs), &dwBytes, nullptr, nullptr))
				throw "��ȡGetAcceptExSockAddrs������ַʧ��";
		}

	}
	bool postAccept(IO_DATA_BASE *pIO_DATA)
	{
		if (!_AcceptEx)
		{
			printf("error, postAccept _AcceptEx is null\n");
			return false;
		}
		pIO_DATA->type = IO_TYPE::ACCEPT;
		pIO_DATA->sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (INVALID_SOCKET == _Listener->_sockAccept)
		{
			printf("��������Accept��Socketʧ�ܣ��������: %d\n", WSAGetLastError());
			return false;
		}
		// Ͷ��AcceptEx
		if (FALSE == _AcceptEx(
			_ListenSocket,
			pIO_DATA->sockfd,
			pIO_DATA->buffer,					//��ſͻ��˵�ַ��Ϣ����Ϣ�Ļ�����
			0,							//���ܵ�һ����Ϣ�ĳ���(�����С - (sizeof(SOCKADDR_IN) + 16) * 2)   Ϊ��ֻ��������
			sizeof(SOCKADDR_IN) + 16,
			sizeof(SOCKADDR_IN) + 16,
			nullptr,					//ͬ��ģʽ�½��յ����ֽ���
			&pIO_DATA->overlapped))			//
		{
			int err = WSAGetLastError();
			if (WSA_IO_PENDING != err)
			{
				printf("Ͷ�� AcceptEx ����ʧ�ܣ��������: %d\n", err);
				return false;
			}
		}
		return true;

	}

	bool postRecv(IO_DATA_BASE *pIO_DATA)
	{
		pIO_DATA->type = IO_TYPE::RECR;
		WSABUF wsbuf = {};
		wsbuf.buf = pIO_DATA->buffer;
		wsbuf.len = DATA_BUFF_SIZE;
		DWORD flags = 0;
		int nBytesRecv = WSARecv(pIO_DATA->sockfd,		//����������׽���
								&wsbuf,					//����������
								1,						//�����������С
								nullptr,				//ͬ��ģʽ�µĴ����ֽ���
								&flags,					//��־
								&pIO_DATA->overlapped,	//�ص���
								nullptr);				//ͬ���ص�
		// �������ֵ���󣬲��Ҵ���Ĵ��벢����Pending�Ļ����Ǿ�˵������ص�����ʧ����
		if ((SOCKET_ERROR == nBytesRecv) && (WSA_IO_PENDING != (nBytesRecv = WSAGetLastError())))//
		{
			if (WSAEFAULT == nBytesRecv)
				printf("Ͷ��һ��WSARecvʧ�� ϵͳ�ڳ���ʹ�õ��õ�ָ�����ʱ��⵽��Ч��ָ���ַ\n");
			else
				printf("Ͷ�ݵ�һ��WSARecvʧ�� Error: %d\n", nBytesRecv);
			return false;
		}
		return true;
	}

	bool postSend(IO_DATA_BASE *pIO_DATA)
	{
		pIO_DATA->type = IO_TYPE::SEND;
		WSABUF wsbuf = {};
		wsbuf.buf = pIO_DATA->buffer;
		wsbuf.len = DATA_BUFF_SIZE;
		DWORD flags = 0;
		int nBytesRecv = WSASend(pIO_DATA->sockfd,		//����������׽���
			&wsbuf,					//����������
			1,						//�����������С
			nullptr,				//ͬ��ģʽ�µĴ����ֽ���
			flags,					//��־
			&pIO_DATA->overlapped,	//�ص���
			nullptr);				//ͬ���ص�
		// �������ֵ���󣬲��Ҵ���Ĵ��벢����Pending�Ļ����Ǿ�˵������ص�����ʧ����
		if ((SOCKET_ERROR == nBytesRecv) && (WSA_IO_PENDING != (nBytesRecv = WSAGetLastError())))//
		{
			if (WSAEFAULT == nBytesRecv)
				printf("Ͷ��һ��WSARecvʧ�� ϵͳ�ڳ���ʹ�õ��õ�ָ�����ʱ��⵽��Ч��ָ���ַ\n");
			else
				printf("Ͷ�ݵ�һ��WSARecvʧ�� Error: %d\n", nBytesRecv);
			return false;
		}
		return true;
	}

	//�ȴ���Ϣ���, ��ȡһ���¼�
	int wait(IO_EVENT & ioEvent, int time)
	{
		memset(&ioEvent, 0, sizeof(IO_EVENT));
		if (!GetQueuedCompletionStatus(_PortHandle, 
										&ioEvent.btesTrans, 
										(PULONG_PTR)&ioEvent.socket, 
										(LPOVERLAPPED*)&ioEvent.pIoData, 
										time))
		{
			int err = GetLastError();
			if (WAIT_TIMEOUT == err)
			{
				return 0;
			}
			if (ERROR_NETNAME_DELETED == err)
			{
				return 1;
			}
			printf("GetQueuedCompletionStatus failed with error %d\n", err);
			return -1;
		}

		return 1;
	}

	//��ӵ��˿�
	bool AddSocket(Event *sockevent)// ����׽���
	{
		if (!CreateIoCompletionPort((HANDLE)sockevent->_socket, _PortHandle, (ULONG_PTR)sockevent, NULL))
			return false;
		//_arrEvent.emplace_back(ev);
		return PostRecv(sockevent);
	}
	
	//���ͽ�����������
	bool PostAccept()
	{
		assert(INVALID_SOCKET != _Listener->_socket);


		// Ϊ�Ժ�������Ŀͻ�����׼����Socket( ������봫ͳaccept�������� ) 
		_Listener->_sockAccept = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (INVALID_SOCKET == _Listener->_sockAccept)
		{
			printf("��������Accept��Socketʧ�ܣ��������: %d\n", WSAGetLastError());
			return false;
		}

		// Ͷ��AcceptEx
		if (FALSE == _AcceptEx(
			_Listener->_socket,
			_Listener->_sockAccept,
			acceptBuf,					//��ſͻ��˵�ַ��Ϣ����Ϣ�Ļ�����
			0,							//���ܵ�һ����Ϣ�ĳ���(�����С - (sizeof(SOCKADDR_IN) + 16) * 2)   Ϊ��ֻ��������
			sizeof(SOCKADDR_IN) + 16,
			sizeof(SOCKADDR_IN) + 16,
			nullptr,					//ͬ��ģʽ�½��յ����ֽ���
			_Listener.get()))			//
		{
			int err = WSAGetLastError();
			if (WSA_IO_PENDING != err)
			{
				printf("Ͷ�� AcceptEx ����ʧ�ܣ��������: %d\n", err);
				return false;
			}
		}
		return true;
	}


	//���ͽ�����������
	bool PostRecv(Event *sockevent)
	{
		DWORD dwFlags = 0, Bytes = 0;
		//sockevent->ResetBuffer();
		sockevent->SetRecvWasBuf();//������ɶ˿ڽ��ջ�����	
		if (_postRecv)
			_postRecv(sockevent);
		//printf("count<%d>RecvBuf<%p>, Size<%d>\n", ++count, sockevent->_Buf.buf, sockevent->_Buf.len);
		sockevent->type = REQUEST_TYPE::RECV;
		int nBytesRecv = WSARecv(sockevent->_socket, &sockevent->_RecvBuf, 1, &Bytes, &dwFlags, sockevent, NULL);
		
		// �������ֵ���󣬲��Ҵ���Ĵ��벢����Pending�Ļ����Ǿ�˵������ص�����ʧ����
		if ((SOCKET_ERROR == nBytesRecv) && (WSA_IO_PENDING != (nBytesRecv = WSAGetLastError())))//
		{
			if (WSAEFAULT == nBytesRecv)
				printf("Ͷ��һ��WSARecvʧ�� ϵͳ�ڳ���ʹ�õ��õ�ָ�����ʱ��⵽��Ч��ָ���ַ\n");
			else
				printf("Ͷ�ݵ�һ��WSARecvʧ�� Error: %d\n", nBytesRecv);
			return false;
		}
		return true;
	}
	
	//���ͷ�����������
	bool PostSend(Event *sockevent)
	{
		DWORD dwFlags = 0;
		DWORD dwBytes = 0;		
		sockevent->type = REQUEST_TYPE::SEND;
		int nBytesRecv = WSASend(sockevent->_socket, &sockevent->_SendBuf, 1, &dwBytes, dwFlags, sockevent, NULL);
		if ((SOCKET_ERROR == nBytesRecv) && (WSA_IO_PENDING != WSAGetLastError()))
		{// �������ֵ���󣬲��Ҵ���Ĵ��벢����Pending�Ļ����Ǿ�˵������ص�����ʧ����
			printf("Ͷ�ݵ�һ��WSASendʧ��: %d\n", WSAGetLastError());
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
		{// �������ֵ���󣬲��Ҵ���Ĵ��벢����Pending�Ļ����Ǿ�˵������ص�����ʧ����
			printf("Ͷ�ݵ�һ��WSASendʧ��: %d\n", WSAGetLastError());
			return false;
		}
		return true;
	}
	
	//�����¼��ص�
	void SetDeleteSocket(BACK_CALL deletesocket) { _DeleteSocket = deletesocket; }
	void SetRecvBackCall(BACK_CALL porecv, BACK_CALL dorecv) {
		_reRecv = dorecv; _postRecv  = porecv;
	};

protected:
	virtual bool DoAccpet()//�������
	{
		sockaddr_in* ClientAddr = NULL;
		sockaddr_in* LocalAddr = NULL;
		int remoteLen = sizeof(SOCKADDR_IN), localLen = sizeof(SOCKADDR_IN);


		// ȡ�ÿͻ��˺ͱ��ض˵ĵ�ַ��Ϣ������˳��ȡ���ͻ��˷����ĵ�һ������
		_GetAcceptExSockAddrs(acceptBuf, 0/*pIoContext->m_wsaBuf.len - ((sizeof(SOCKADDR_IN) + 16) * 2)*/,
			sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, (LPSOCKADDR*)&LocalAddr, &localLen, (LPSOCKADDR*)&ClientAddr, &remoteLen);

		if (_acceptCall)
			_acceptCall(_Listener->_sockAccept, *ClientAddr);

		return PostAccept();
	}
	virtual void DoRecv(Event *pEvent)
	{
		pEvent->RecvWasBuf();//���½��ջ�����
		if (_reRecv)
			_reRecv(pEvent);
		if (pEvent->isSendWasBuf())//�Ƿ���Ҫ��������, ��Ҫ��������ɶ˿ڷ��ͻ�����
		{
			if (!PostSend(pEvent))
				_DeleteSocket(pEvent);
		}
		else
		{
			//Ͷ����һ��WSARecv����
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
	//�߳�����
	void Run()
	{		
		DWORD                dwBytesTransfered = 0;
		ULONG_PTR			 KeyValue;//ע���ǻ���PostQueuedCompletionStatus���͹����ļ�ֵ
		LPOVERLAPPED         pOverlapped = NULL;

		int thread = ++count2;
		// ѭ����������֪�����յ�Shutdown��ϢΪֹ
		while (WAIT_OBJECT_0 != WaitForSingleObject(_hShutdownEvent, 0))
		{
			BOOL bReturn = GetQueuedCompletionStatus(
				_PortHandle,			//��ɶ˿ھ��
				&dwBytesTransfered,		//ʵ�ʴ���������ֽ���
				&KeyValue,				//������ɶ˿�ʱ���ݵļ�ֵ
				&pOverlapped,			//������ɶ˿�ʱ���ݵ��ص���
				INFINITE);				//�ȴ�ʱ��, INFINITE���޵ȴ�

			
			// ����յ������˳���־����ֱ���˳�
			if (EXIT_CODE == KeyValue)
			{
				break;
			}
			//Event* pEvent = (Event*)KeyValue;
			Event * pEvent = static_cast<Event*>(pOverlapped);
			// �ж��Ƿ�����˴���
			if (!bReturn)
			{
				DWORD dwErr = GetLastError();
				// ��ʾһ����ʾ��Ϣ
				// ����ǳ�ʱ�ˣ����ټ�����
				if (WAIT_TIMEOUT == dwErr)
				{
					// ȷ�Ͽͻ����Ƿ񻹻���...
					if (-1 == send(pEvent->_socket, "", 0, 0))
					{
						//printf("��ʱ, ��⵽�ͻ����쳣�˳���\n");
						_DeleteSocket(pEvent);
					}
				}
				// �����ǿͻ����쳣�˳�
				else if (ERROR_NETNAME_DELETED == dwErr)
				{
					//printf("��⵽�ͻ����쳣�˳���\n");
					_DeleteSocket(pEvent);
				}
				else if (ERROR_CONNECTION_ABORTED == dwErr)
				{
					//printf("�������ӱ�����ϵͳ��ֹ\n");
					_DeleteSocket(pEvent);
				}
				else
				{
					//printf("��ɶ˿ڲ������ִ����߳��˳���������룺%d\n", dwErr);		
					printf("��ɶ˿ڲ�������δ֪���󡣴�����룺%d\n", dwErr);
				}

				continue;
			}
			else
			{
				// ��ȡ����Ĳ���
				//Event* pIoContext = CONTAINING_RECORD(pOverlapped, Event, OVERLAPPED);
				pEvent->Bytes = dwBytesTransfered;		
				if ((0 == dwBytesTransfered) && (REQUEST_TYPE::RECV == pEvent->type || REQUEST_TYPE::SEND == pEvent->type))
				{// �ж��Ƿ��пͻ��˶Ͽ���
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
						printf("IOCPδ֪��������.\n");
						break;
					}
					
		
				}//if
			}//if

		}//while

	}
};
#endif
#endif


