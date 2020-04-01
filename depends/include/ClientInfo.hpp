#ifndef CLIENT_INFO_HPP
#define CLIENT_INFO_HPP
#include"GlobalDef.hpp"
#include"CellBuffer.hpp"
//定时发送间隔
#define CLIENT_SEND_BUFF_TIME 300

class CellServer;
class EasyTcpServer;
//客户端数据类型
class ClientInfo
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
public:
	SOCKET _socket;
	sockaddr_in addr;
public:
	ClientInfo() :_socket(0), addr({}), sendBuf(SEND_BUFF_SIZE),
		recvBuf(RECV_BUFF_SIZE), _dtHeart(0), _dtSend(0)
	{
		resetDTHeart();
	}
	~ClientInfo()
	{
		if (INVALID_SOCKET != _socket)
		{
#ifdef _WIN32
			closesocket(_socket);
#else
			close(_socket);
#endif
			_socket = INVALID_SOCKET;
		}

	}

	int RecvData()
	{
		return recvBuf.read4socket(_socket);
	}

	//是否有数据可处理
	bool hasMsg()
	{
		return recvBuf.hasMsg();
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
	int SendData(DataHeader* pheader)
	{
		if (sendBuf.push((const char*)pheader, pheader->datalength))
			return pheader->datalength;

		return SOCKET_ERROR;
	}

	void resetDTHeart()
	{
		_dtHeart = 0;
	}

	bool checkHeart(time_t dt)
	{
		_dtHeart += dt;
		if (_dtHeart >= CLIENT_HREAT_DEAD_TIME)
		{
			xPrintf("checkHeart dead:s = %d, time = %d\n", (int)_socket, (int)_dtHeart);
			return true;
		}
	
		return false;
	}

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

