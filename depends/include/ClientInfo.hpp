#ifndef CLIENT_INFO_HPP
#define CLIENT_INFO_HPP
#include"GlobalDef.hpp"
#include"CellBuffer.hpp"
//定时发送间隔
#define CLIENT_SEND_BUFF_TIME 300

class CellServer;
class EasyTcpServer;
//客户端数据类型WSASOVERLAPPED
#ifdef _WIN32
#include "IOCP.hpp"
class ClientInfo : public Event
#else
public:
	SOCKET _socket;
	sockaddr_in _addr;
class ClientInfo
#endif // _Win32
{
	friend CellServer;
	friend EasyTcpServer;
private:
	//发送缓冲区
	CellBuffer sendBuf;
	//接收缓冲区
	CellBuffer recvBuf;
	//心跳死亡计时
	time_t _dtHeart;
	//发送计时
	time_t _dtSend;
	//连接、登录时间
	std::atomic_llong _dtconnect_login;
	//是否登录
	std::atomic_bool _islongin;
public:
	char *_id, *_password;
	bool _isadmin;
	//验证码计时
	time_t _dtsecurity;
	//验证码
	std::string security;
public:
	ClientInfo(SOCKET sock = INVALID_SOCKET) :  sendBuf(SEND_BUFF_SIZE),
		recvBuf(RECV_BUFF_SIZE), _dtHeart(0), _dtSend(0), _dtconnect_login(0), _islongin(false),
		_id(nullptr), _password(nullptr), _isadmin(false), _dtsecurity(0)
	{
		resetDTHeart();
		_id = new char[32]();
		_password = new char[32]();
		_socket = sock;
#ifndef _WIN32	
		memset(&_addr, 0, sizeof sockaddr_in)
#endif
	}
	~ClientInfo()
	{
		CloseSock();
		if (_id)
			delete[] _id;
		if (_password)
			delete[] _password;
	}
	//关闭套接字
	void CloseSock()
	{
		if (INVALID_SOCKET != _socket)
		{
#ifdef _WIN32
			closesocket(_socket);
#else
			close(sock);
#endif
			_socket = INVALID_SOCKET;
		}
	}
	//从套接字接收数据
	int RecvData()
	{
		return recvBuf.read4socket(_socket);
	}
#ifdef _WIN32
	//
	std::string Base64(std::string str)
	{	
		int len = str.size() * 1.4;
		char * tmp = new char[len] {};
		memcpy(tmp, str.data(), str.size());
		len = sendBuf.base64(tmp, str.size());
		delete[] tmp;
		return std::string(tmp, len);
	}
	//设置完成端口接收缓冲区
	void SetRecvWasBuf()
	{
		recvBuf.GetBufandLen(_RecvBuf.buf, _RecvBuf.len);
	}
	//报告实际接收到的字节数, 并解密
	void RecvWasBuf()
	{
		recvBuf.readBuf(_RecvBuf.buf, Bytes);
	}
	//是否需要发送数据, 并设置完成端口发送缓冲区
	bool isSendWasBuf()
	{
		
		int len = sendBuf.Length();
		if (len > 0)
		{
#ifdef _BASE64
			int baselen = sendBuf.base64Data();
			_SendBuf.buf = sendBuf.Data();
			_SendBuf.len = baselen;
#else
			_SendBuf.buf = sendBuf.Data();
			_SendBuf.len = len;
#endif
			
			return true;
		}
		return false;
	}
	void ResetSendBuf()
	{
		sendBuf.ResetBuf();
	}

#endif
	//是否有数据包处理
	bool hasMsg()
	{
		return recvBuf.hasMsg();
	}
	//发送缓冲区有数据
	bool needWrite()
	{
		return sendBuf.needWrite();
	}
	//返回接收缓冲区第一个消息
	DataHeader* front()
	{
		return (DataHeader*)recvBuf.Data();
	}
	//删除第一个消息
	void pop_front()
	{
		if (recvBuf.hasMsg())
			recvBuf.pop();
	}
	//定时发送  
	int SendDateReal()
	{
		_dtSend = 0;
		return sendBuf.write2socket(_socket);
	}
	//发送
	int SendData(char * pheader, int len)
	{
		if (sendBuf.push((const char*)pheader, len))
			return len;

		return SOCKET_ERROR;
	}
	//发送数据包
	int SendData(DataHeader * pheader)
	{
		if (sendBuf.push((const char*)pheader, pheader->datalength))
			return pheader->datalength;

		return SOCKET_ERROR;
	}
	//立即发送数据包
	int Send(DataHeader * pheader)
	{
		return send(_socket, (char*)pheader, pheader->datalength, 0);
	}
	//心跳时间归零
	void resetDTHeart()
	{
		_dtHeart = 0;
	}
	//检查是否超时、踢掉客户端
	bool checkClient(time_t &dt, time_t &now)
	{
#ifdef CLIENT_HREAT_DEAD_TIME
		_dtHeart += dt;
		if (_dtHeart >= CLIENT_HREAT_DEAD_TIME) //检查心跳是否超时
		{
			xPrintf("<%d>checkTimeout socket: %d\n", (int)_socket, (int)_dtHeart);
			return true;
		}
		if (!_islongin && now - _dtconnect_login >= CLIENT_HREAT_DEAD_TIME)//检查是否未登录 未登录超时返回真
		{
			xPrintf("<%d>unLogin %d\n", (int)_socket, (int)_dtconnect_login);
			return true;
		}
#endif // CLIENT_HREAT_DEAD_TIME

		return false;
	}
	//设置登录状态
	void SetLoginStatus(bool status)
	{		
		_dtconnect_login = CELLTime::getNowinMilliSec();
		_islongin = status;			
	}	
	//定时发送 检查是否要发送
	bool checkSend(time_t dt)
	{
		_dtSend += dt;
		if (_dtSend >= CLIENT_SEND_BUFF_TIME)
		{			
			SendDateReal();		
			return true;
		}

		return false;
	}

};
#endif // !1

