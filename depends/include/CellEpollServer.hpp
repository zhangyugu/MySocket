#ifndef CELL_EPOLL_SERVER_HPP
#define CELL_EPOLL_SERVER_HPP
#include"GlobalDef.hpp"
#include "CellServer.hpp"
#include"Epoll.hpp"

#if __linux__
class CellEpollServer : public CellServer
{

public:

	explicit CellEpollServer(INetEvent *eventObj = nullptr) : CellServer(eventObj) {}
	CellEpollServer(const CellEpollServer&) = delete;
	CellEpollServer& operator= (const CellEpollServer&) = delete;
	bool doNetEvent()
	{
		int ret = _epoll.wait(1);
		if (ret < 0)
		{

			return false;
		}
		else if (ret == 0)
			return true;
	
		return true;
	}
	void OnclientJoin(ClientInfo * pclient)
	{
		_epoll.add(pclient->_socket, pclient, EPOLLIN);
	}
private:
	Epoll _epoll;

};
#endif //__linux__
#endif // !CELL_SERVER_HPP