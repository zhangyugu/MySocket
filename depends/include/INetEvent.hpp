#ifndef I_NET_EVENT_HPP
#define I_NET_EVENT_HPP
#include"GlobalDef.hpp"
class ClientInfo;
class CellServer;

//网络事件接口
class INetEvent
{
public:
	//纯虚函数 抽象类    
	//客户端退出
	virtual void OnNetLeave(ClientInfo* pClient) = 0;
	//客户端加入
	virtual void OnNetJoin(ClientInfo* pClient) = 0;
	//接收技术
	virtual void OnNetRevc(ClientInfo* pClient) = 0;
	//计数
	virtual void OnNetMsg(CellServer *pCellServer, ClientInfo* pClient) = 0;

};
#endif // !I_NET_EVENT_HPP
