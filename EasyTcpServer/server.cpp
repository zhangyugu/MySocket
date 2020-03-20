#include "EasyTcpServer.hpp"
#include <vector>
#include"CellMsgStream.hpp"


class MyServer : public EasyTcpServer
{
public:
	MyServer() {}
	~MyServer() {}
	virtual void OnNetLeave(ClientInfo* pClient)
	{
		//执行处理
		
		EasyTcpServer::OnNetLeave(pClient);
		//printf("客户端退出\n");
	}
	virtual void OnNetJoin(ClientInfo* pClient)
	{
		//执行处理
		EasyTcpServer::OnNetJoin(pClient);
		
		//printf("客户端加入\n");
	}
	virtual void OnNetRevc(ClientInfo* pClient)
	{
		EasyTcpServer::OnNetRevc(pClient);
	}
	virtual void OnNetMsg(CellServer *pCellServer, ClientInfo* pClient)
	{
		EasyTcpServer::OnNetMsg(pCellServer, pClient);
		switch (pClient->front()->cmd)
		{
		case CMD::HEART_C:
		{	
			pClient->resetDTHeart();

			DataHeader msg;
			msg.cmd = CMD::HEART_S;
			msg.datalength = sizeof(DataHeader);
			if (SOCKET_ERROR == pClient->SendData(&msg))
			{
				printf("<Socket=%d> Send Full\n", (int)pClient->_socket);
			}
		}
		break;
		case CMD::LOGIN:
		{
			//接收包数据
			CellMsgStream s(pClient->front());
			//Login_out *login_out = (Login_out *)pheader;
			char id[32], PassWord[32], tmp[10];

			int8_t int8 = s.read_int8();
			int16_t int16 = s.read_int16();
			int32_t int32 = s.read_int32();
			int64_t int64 = s.read_int64();

			//s.readCstring(id);
			s.read_array(id, 32);
			uint8_t  uint8 = s.read_uint8();
			uint16_t uint16 = s.read_uint16();
			uint32_t uint32 = s.read_uint32();
			uint64_t uint64 = s.read_uint64();
			s.read_array(tmp, 10);
			float f = s.read_float();
			double d = s.read_double();
			
			s.read_array(PassWord, 32);
			std::cout << "登录命令: " << id << "," << PassWord;
			std::cout << std::endl;
			//不验证账号密码  直接发送成功状态

			CellMsgStream  send;
			send.setNetCmd(CMD::LOGIN_RESULT);
			send.write_int16((short)STATUS::SUCCEED);
			/*send.cmd = CMD_LOGIN_RESULT;
			send.datalength = sizeof(Login_out);
			send.information = STATUS_SUCCEED;*/

			if (SOCKET_ERROR == pClient->SendData(send.getheader()))
			{
				//发送缓冲区满了
				printf("发送缓冲区满了\n");
			}

			/*Login_out  *send = new Login_out;
			send->cmd = CMD_LOGIN_RESULT;
			send->datalength = sizeof(Login_out);
			send->information = STATUS_SUCCEED;*/


			//std::shared_ptr<DataHeader> tmpptr(send);

			//pCellServer->addTask(pClient, send);
			//pClient->SendData(send);
			//SendData(pinfo, login_out);
		}
		break;
		case CMD::LOGOUT:
		{
			////接收包数据
			//Login_out * login_out = (Login_out *)pheader;
			//std::cout << "登录命令: " << login_out->userName << "," << login_out->PassWord << std::endl;
			////不验证账号密码  直接发送成功状态
			//login_out->cmd = CMD_LOGOUT_RESULT;
			//login_out->datalength = sizeof(Login_out);
			//login_out->information = STATUS_SUCCEED;
			//SendData(&pinfo->_socket, login_out);
		}
		break;
		default:
		{
			std::cout << "未知命令 " << std::endl;
		}
		break;
		}
	}

private:

};




int main()
{
	Log::Instance()->setLogPath("server.txt", "w");
	MyServer server;

	server.CreateListenSocket(4567, 5);
#ifdef _WIN32
	server.Start();
#else
	server.Start(4);
#endif

	Login_out login_out;
	login_out.cmd = CMD::LOGIN_RESULT;
	login_out.datalength = sizeof(Login_out);
	login_out.information = (short)STATUS::SUCCEED;

	char msgBuf[128] = {};
	while (true)
	{
		//3 发送请求
		//system("cls"); //清空命令行  UNIX/Linux system("clear");
		//std::cout << "输入的操作命令: ";
		std::cin >> msgBuf;
		getchar();
		if (0 == strcmp(msgBuf, "exit"))
		{
			break;
		}
		else
		{
			std::cout << "未知命令: " << std::endl;
			continue;
		}
	}
	server.Close();


	std::cout << "退出进程!\n" << std::endl;
	//getchar();
	return 0;
}
