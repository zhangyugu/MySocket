#include<iostream>
#include<vector>
#include<fstream>
#include<WinSock2.h>
#include<Windows.h> 
#include<tchar.h>
using namespace std;
#define BUF_TIMES 10
struct CompletionKey{ SOCKET s; };
typedef struct io_operation_data
{
	WSAOVERLAPPED overlapped; //重叠结构  
	WSABUF wsaBuf; //发送接收缓冲区  
}IO_OPERATION_DATA, *PIO_OPERATION_DATA;
int main2()
{
	HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	if (FAILED(hr))
	{
		MessageBox(NULL, _T("Initialize COM failed!"), _T("error"), MB_OK);
		//cout<<"Initialize COM failed!"<<endl;  
		//return false;  
	}
	WORD wVersionRequest;
	WSADATA wsaData;
	wVersionRequest = MAKEWORD(2, 2);
	int nErrCode = WSAStartup(wVersionRequest, &wsaData);
	if (nErrCode != 0)
	{
		//cout<<" start up error!"<<endl;  
		MessageBox(NULL, _T("start up error!"), _T("error"), MB_OK);
	}
	if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
	{
		//cout<<" lib is not 2.2!"<<endl;  
		MessageBox(NULL, _T("lib is not 2.2!"), _T("error"), MB_OK);
		WSACleanup();
	}
	SOCKET sListen = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);//socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);  
	if (INVALID_SOCKET == sListen)
	{
		MessageBox(NULL, _T("socket error!"), _T("error"), MB_OK);
	}

	//获取系统默认的发送数据缓冲区大小  
	unsigned int uiRcvBuf;
	int uiRcvBufLen = sizeof(uiRcvBuf);
	nErrCode = getsockopt(sListen, SOL_SOCKET, SO_SNDBUF, (char*)&uiRcvBuf, &uiRcvBufLen);
	if (SOCKET_ERROR == nErrCode)
	{
		//cout<<"获取系统默认的发送数据缓冲区大小failed!"<<endl;  
		MessageBox(NULL, _T("获取系统默认的发送数据缓冲区大小failed!"), _T("error"), MB_OK);
		//return false;  
	}
	//设置系统发送数据缓冲区为默认值的BUF_TIMES倍  
	uiRcvBuf *= BUF_TIMES;
	nErrCode = setsockopt(sListen, SOL_SOCKET, SO_SNDBUF, (char*)&uiRcvBuf, uiRcvBufLen);
	if (SOCKET_ERROR == nErrCode)
	{
		//cout<<"修改系统发送数据缓冲区失败！"<<endl;  
		MessageBox(NULL, _T("修改系统发送数据缓冲区失败！"), _T("error"), MB_OK);
	}
	//检查设置系统发送数据缓冲区是否成功  
	unsigned int uiNewRcvBuf;
	nErrCode = getsockopt(sListen, SOL_SOCKET, SO_SNDBUF, (char*)&uiNewRcvBuf, &uiRcvBufLen);
	if (SOCKET_ERROR == nErrCode || uiNewRcvBuf != uiRcvBuf)
	{
		//      cout<<"修改系统发送数据缓冲区失败！"<<endl;  
		MessageBox(NULL, _T("修改系统发送数据缓冲区失败！"), _T("error"), MB_OK);
	}
	//cout<<uiNewRcvBuf<<endl;  
	HANDLE CompPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (CompPort == NULL)
	{
		MessageBox(NULL, _T("创建完成端口失败!"), _T("error"), MB_OK);
		//WSACleanup();  
		//return ;  
	}
	SOCKADDR_IN addrHost;
	addrHost.sin_family = AF_INET;
	addrHost.sin_port = htons(4567);
	addrHost.sin_addr.S_un.S_addr = INADDR_ANY;
	int retVal = bind(sListen, (LPSOCKADDR)&addrHost, sizeof(SOCKADDR_IN));
	if (SOCKET_ERROR == retVal)
	{
		//cout<<"bind error!"<<endl;  
		MessageBox(NULL, _T("bind error!"), _T("error"), MB_OK);
		//closesocket(this->m_sListen);  
		//return false;  
	}
	retVal = listen(sListen, 5);
	if (SOCKET_ERROR == retVal)
	{
		MessageBox(NULL, _T("listen error!"), _T("error"), MB_OK);

	}
	sockaddr_in addrClient;
	int addrLen = sizeof(sockaddr_in);
	SOCKET sClient = accept(sListen, (sockaddr*)&addrClient, &addrLen);

	cout << "come in" << endl;
	CompletionKey iCompletionKey;
	iCompletionKey.s = sClient;
	//关联完成端口  
	if (CreateIoCompletionPort((HANDLE)sClient, CompPort, (DWORD)(&iCompletionKey), 0) == NULL)
	{
		//出错处理。。  
		MessageBox(NULL, _T("关联完成端口失败!"), _T("error"), MB_OK);
	}
	DWORD dwIoSize = -1; //传输字节数  
	LPOVERLAPPED lpOverlapped = NULL; //重叠结构指针  
	CompletionKey* pCompletionKey = NULL;
	DWORD flags = 0; //标志  
	DWORD recvBytes = 0; //接收字节数  
	PIO_OPERATION_DATA pIO = NULL;
	int count = 0;
	pIO = new IO_OPERATION_DATA;
	ZeroMemory(pIO, sizeof(IO_OPERATION_DATA));
	pIO->wsaBuf.buf = new char[10240 * 5];
	pIO->wsaBuf.len = 10240 * 5;
	if (WSARecv(sClient, &(pIO->wsaBuf), 1, &recvBytes, &flags, &(pIO->overlapped), NULL) == SOCKET_ERROR)
	{
		if (ERROR_IO_PENDING != WSAGetLastError())
		{
			cout << "recv failed" << endl;
		}
	}
	vector<IO_OPERATION_DATA*> vecCache;
	ofstream fos;
	fos.open("s.txt");
	while (1)
	{
		BOOL bIORet = GetQueuedCompletionStatus(CompPort,
			&dwIoSize,
			(LPDWORD)&pCompletionKey,
			&lpOverlapped,
			INFINITE);
		//失败的操作完成  
		if (FALSE == bIORet && NULL != pCompletionKey)
		{
			//客户端断开   
		}
		//成功的操作完成  
		if (bIORet && lpOverlapped && pCompletionKey)
		{
			pIO  = CONTAINING_RECORD(lpOverlapped, IO_OPERATION_DATA, overlapped);
			if (dwIoSize < pIO->wsaBuf.len)
			{
				vecCache.push_back(pIO);
				int size = pIO->wsaBuf.len;
				pIO->wsaBuf.len = dwIoSize;
				pIO = new IO_OPERATION_DATA;
				ZeroMemory(pIO, sizeof(IO_OPERATION_DATA));
				pIO->wsaBuf.buf = new char[size - dwIoSize];
				pIO->wsaBuf.len = size - dwIoSize;
				if (WSARecv(sClient, &(pIO->wsaBuf), 1, &recvBytes, &flags, &(pIO->overlapped), NULL) == SOCKET_ERROR)
				{
					if (ERROR_IO_PENDING != WSAGetLastError())
					{
						cout << "recv failed" << endl;
					}
				}
			}
			else 
			{
				if (vecCache.size() != 0)
				{
					char*p = new char[10240 * 5];
					int size = 0;
					for (vector<IO_OPERATION_DATA*>::iterator it = vecCache.begin();
						it != vecCache.end(); it++)
					{
						memcpy(p + size,(*it)->wsaBuf.buf,(*it)->wsaBuf.len);
						size += (*it)->wsaBuf.len;
						//清理资源  
						delete []((*it)->wsaBuf.buf);
						delete (*it);
					}
					memcpy(p + size,pIO->wsaBuf.buf,pIO->wsaBuf.len);
					count++;
					fos.write(p,10240 * 5);
					fos << endl;
					vecCache.clear();
					cout << "count: " << count << endl;
					//清理资源  
					delete [](pIO->wsaBuf.buf);
					delete pIO;
					delete []p;
				}
				else 
				{
					count++;
					fos.write(pIO->wsaBuf.buf,10240 * 5);
					fos << endl;
					cout << "count: " << count 
						<< "  number: " << dwIoSize << endl;
					//清理资源  
					delete [](pIO->wsaBuf.buf);
					delete pIO;
				}
					pIO = new IO_OPERATION_DATA;
					ZeroMemory(pIO, sizeof(IO_OPERATION_DATA));
					pIO->wsaBuf.buf = new char[10240 * 5];
					pIO->wsaBuf.len = 10240 * 5;
					if (WSARecv(sClient,&(pIO->wsaBuf),1,&recvBytes,&flags,&(pIO->overlapped),NULL) == SOCKET_ERROR)
					{
						if (ERROR_IO_PENDING != WSAGetLastError())
						{
							cout << "recv failed" << endl;
						}
					}

			}
				if (count == 100)
					break;
		}
	}

	cout << "main end: " << count << endl;
	fos.close();
	Sleep(10000);
	CoUninitialize();
	WSACleanup();
	return 0;
}
