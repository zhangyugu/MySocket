#include<iostream>  
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

SOCKET sClient;
//char (*data)[1024];  
DWORD WINAPI ServiceThreadProc(PVOID pParam);
int main1()
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
	sClient = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);//socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);  
	if (INVALID_SOCKET == sClient)
	{
		MessageBox(NULL, _T("socket error!"), _T("error"), MB_OK);
	}

	//获取系统默认的发送数据缓冲区大小  
	unsigned int uiRcvBuf;
	int uiRcvBufLen = sizeof(uiRcvBuf);
	nErrCode = getsockopt(sClient, SOL_SOCKET, SO_SNDBUF, (char*)&uiRcvBuf, &uiRcvBufLen);
	if (SOCKET_ERROR == nErrCode)
	{
		//cout<<"获取系统默认的发送数据缓冲区大小failed!"<<endl;  
		MessageBox(NULL, _T("获取系统默认的发送数据缓冲区大小failed!"), _T("error"), MB_OK);
		//return false;  
	}
	//设置系统发送数据缓冲区为默认值的BUF_TIMES倍  
	uiRcvBuf *= BUF_TIMES;
	nErrCode = setsockopt(sClient, SOL_SOCKET, SO_SNDBUF, (char*)&uiRcvBuf, uiRcvBufLen);
	if (SOCKET_ERROR == nErrCode)
	{
		//cout<<"修改系统发送数据缓冲区失败！"<<endl;  
		MessageBox(NULL, _T("修改系统发送数据缓冲区失败！"), _T("error"), MB_OK);
	}

	//检查设置系统发送数据缓冲区是否成功  
	unsigned int uiNewRcvBuf;
	nErrCode = getsockopt(sClient, SOL_SOCKET, SO_SNDBUF, (char*)&uiNewRcvBuf, &uiRcvBufLen);
	if (SOCKET_ERROR == nErrCode || uiNewRcvBuf != uiRcvBuf)
	{
		//      cout<<"修改系统发送数据缓冲区失败！"<<endl;  
		MessageBox(NULL, _T("修改系统发送数据缓冲区失败！"), _T("error"), MB_OK);
	}
	HANDLE CompPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (CompPort == NULL)
	{
		MessageBox(NULL, _T("创建完成端口失败!"), _T("error"), MB_OK);
		//WSACleanup();  
		//return ;  
	}
	CompletionKey iCompletionKey;
	iCompletionKey.s = sClient;
	if (CreateIoCompletionPort((HANDLE)sClient, CompPort, (DWORD)&iCompletionKey, 0) == NULL)
	{
		//出错处理。。  
		MessageBox(NULL, _T("关联完成端口失败!"), _T("error"), MB_OK);
	}
	//addrHost    
	SOCKADDR_IN addrHost;
	addrHost.sin_family = AF_INET;
	addrHost.sin_port = htons(4567);
	addrHost.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");

	int retVal = connect(sClient, (sockaddr*)&addrHost, sizeof(sockaddr));
	if (retVal == SOCKET_ERROR)
	{
		MessageBox(NULL, _T("connect error!"), _T("error"), MB_OK);
		//return false;  
	}
    cout << "connect success" << endl;

	HANDLE hThread = CreateThread(NULL, 0, ServiceThreadProc, NULL, 0, NULL);

	DWORD dwIoSize = -1; //传输字节数  
	LPOVERLAPPED lpOverlapped = NULL; //重叠结构指针  
	CompletionKey* pCompletionKey = NULL;
	PIO_OPERATION_DATA pIO = NULL;
	int count = 0;
	ofstream fos;
	fos.open("c.txt");

	while (1)
	{
		BOOL bIORet = GetQueuedCompletionStatus(CompPort,
			&dwIoSize,(LPDWORD)&pCompletionKey,
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
			cout << "count: " << count + 1 << "  "
				<< "number: " << dwIoSize << endl;
			pIO = CONTAINING_RECORD(lpOverlapped, IO_OPERATION_DATA, overlapped);
			fos.write(pIO->wsaBuf.buf, 10240 * 5);
			fos << endl;
			if (count == 99)
				break;
			count++;
		}

	}

	WaitForSingleObject(hThread, INFINITE);
	cout << "main end: " << count + 1 << endl;
	fos.close();
	Sleep(5000);
	CoUninitialize();
	WSACleanup();
	return 0;
}
DWORD WINAPI ServiceThreadProc(PVOID pParam)
{
	char (*a)[10240 * 5] = new char[100][10240 * 5];
	for (int i = 0; i < 100; i++)
	{
		if (i % 3 == 0)
			memset(a[i], 'a', 10240 * 5);
		else if(i % 3 == 1)
			memset(a[i], 'b', 10240 * 5);
		else if(i % 3 == 2)
			memset(a[i], 'c', 10240 * 5);
	}
	DWORD flags = 0; //标志  
	DWORD sendBytes = 0; //发送字节数  
	int count = 0;
	while (count < 100)
	{
		PIO_OPERATION_DATA pIO = new IO_OPERATION_DATA;
		ZeroMemory(pIO, sizeof(IO_OPERATION_DATA));
		pIO->wsaBuf.buf = a[count];
		pIO->wsaBuf.len = 10240 * 5;

		//Sleep(30);  
		if (WSASend(sClient, &(pIO->wsaBuf), 1, &sendBytes, flags, &(pIO->overlapped), NULL) == SOCKET_ERROR)
		{
			if (ERROR_IO_PENDING != WSAGetLastError())//发起重叠操作失败  
			{
				cout << "send failed" << endl;
			}
		}
		count++;
	}

	cout << "thread func: " << count << endl;
	Sleep(10000);

	return 0;
}
