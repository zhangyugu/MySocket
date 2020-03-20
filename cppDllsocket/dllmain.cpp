#ifndef CPP_DLL_SOCKET_HPP
#define CPP_DLL_SOCKET_HPP
#include "EasyTcpClinet.hpp"
#ifdef _WIN32
	#define EXPORT_DLL _declspec(dllexport)
#else
	#define EXPORT_DLL
#endif
class TcpClient : public EasyTcpClient
{
public:
	TcpClient() {}
	~TcpClient() {}
protected:
	virtual void OnNetMsg()
	{
		EasyTcpClient::OnNetMsg();
	}
private:

};



extern "C"
{
	void* EXPORT_DLL Client_Create(int a, int b)
	{
		TcpClient * pClient = new TcpClient();
		return pClient;
	}

	typedef void(*OnNetMsgCallBack) (const char* str);

	void EXPORT_DLL Client_Connect(void* csObj, OnNetMsgCallBack cb)
	{

	}

}


#endif // !CPP_DLL_SOCKET_CPP
