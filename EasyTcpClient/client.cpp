#include "EasyTcpClinet.hpp"
#include"CellMsgStream.hpp"

class MyClient : public EasyTcpClient
{
public:
	MyClient(){}
	~MyClient() {}
protected:
	virtual void OnNetMsg()
	{
		//Login_out *login_out = (Login_out *)header;		
		//std::cout << front << std::endl;
		switch (pClinet->front()->cmd)
		{
		case CMD::HEART_S:
		{

		}
		break;
		case CMD::LOGIN_RESULT:
		{

			//if (STATUS::SUCCEED != login_out->information)
			/*	std::cout << "<socket = " << _sock << ">登录成功!" << std::endl;
			else*/
			//std::cout << "<socket = " << _sock << ">登录失败!" << std::endl;
		}
		break;
		case CMD::LOGOUT_RESULT:
		{
			/*if (STATUS::SUCCEED == login_out->information)
				std::cout << "<socket = " << _sock << ">退出成功!" << std::endl;
			else
				std::cout << "<socket = " << _sock << ">退出失败!" << std::endl;*/
		}
		break;
		case CMD::NEWUSER_JOIN:
		{
			//std::cout << "新用户加入: socket = " << login_out->information << std::endl;
		}
		break;
		case CMD::STATUS:
		{
			CellMsgStream stream(pClinet->front());
			STATUS status = static_cast<STATUS>(stream.read_int16());
			switch (status)
			{
			case STATUS::SUCCEED:
				//std::cout << "操作成功!" << std::endl;
				break;
			case STATUS::EXIST:
				std::cout << "账号已存在!" << std::endl;
				break;
			case STATUS::NO_EXIST:
				std::cout << "账号不存在!" << std::endl;
				break;
			case STATUS::PASSWORD_ERROR:
				std::cout << "密码错误!" << std::endl;
				break;
			case STATUS::EXPIRE:
				std::cout << "账号已到期!" << std::endl;
				break;
			case STATUS::NO_CARD:
				std::cout << "充值卡不存在!" << std::endl;
				break;
			case STATUS::SUCCEED_CREATE_CAL:
			{
				char cal[33];
				int num = stream.read_int32();
				for (int i = 0; i < num; ++i)
				{
					stream.read_array(cal, 33);
					std::cout << cal << std::endl;
				}
				//STATUS::SUCCEED_CREATE_CAL,
				//STATUE_SECURITY_CARD,		//验证码已过期
				//	STATUE_SECURITY_ERROR		//验证码错误
			}
			break;
			case STATUS::SECURITY_CARD:
				std::cout << "验证码已过期!" << std::endl;
				break;
			case STATUS::SECURITY_ERROR:
				std::cout << "验证码错误!" << std::endl;
				break;
			case STATUS::FAILED:
				std::cout << "未知错误!" << std::endl;
				break;
			case STATUS::SECURITY_FRE:
				std::cout << "验证码获取频繁!" << std::endl;
				break;
			default:
				std::cout << "未知STATUS!" << std::endl;
				break;
			}
			
		}
		break;
		default:
			Log::Info("socket<%d> unknown message!", pClinet->_socket);
			break;
		}

		EasyTcpClient::OnNetMsg();
	}
private:

};


bool g_Run = true;
void cmdThread()
{
	char msgBuf[128] = {};
	while (true)
	{
		//3 发送请求
		//system("cls"); //清空命令行  UNIX/Linux system("clear");
		//std::cout << "输入的操作命令: ";
		std::cin >> msgBuf;
		if (0 == strcmp(msgBuf, "exit"))
		{
			g_Run = false;
			std::cout << "退出 cmdThread 线程!" << std::endl;
			getchar();
			break;
		}
		else
		{
			std::cout << "未知命令: " << std::endl;
			continue;
		}
	}

}

char IP[MAX_PATH]; 

int tCount = 0;
int sCount = 0;
std::atomic_int cCount = 0;//客户端计数

std::atomic_int readyCount = 0;
//EasyTcpClient *client[sCount];

void RecvThread(std::vector<EasyTcpClient *> *clients)
{
	//CELLTimestamp tTime;

	//while (g_Run)
	//{
	//	for (auto it = clients->begin(); it != clients->end();)
	//	{
	//		if ((*it)->OnRunLoop())
	//		{

	//			delete *it;
	//			*it = nullptr;
	//			it = clients->erase(it);
	//			--cCount;
	//			continue;
	//		}

	//		++it;
	//	}
	//}
}

void clientThread(int index)
{
	int clientNum = sCount / tCount;
	std::vector<EasyTcpClient *> clients;
	for (int i = 0; i < clientNum; i++)
	{
		auto tmp = new MyClient;
		if (tmp)
		{
			//CELLTimestamp tmpTime;
			if (tmp->Connect(IP, 4567))
			{
				//printf("连接时间: %lf\n", tmpTime.getElapsedSecond());
				 
				clients.push_back(tmp);
				++cCount;
			}
			else
			{
				std::cout << "连接失败..." << std::endl;
				delete tmp;
			}
		}
	}
		

	
	//192.168.2.100
	//linux 192.168.184.154 max 192.168.184.155
	//for (auto &x : clients)
	//{
	//	//std::cout << "连接计数: " << i + 1 << std::endl;
	//	x
	//}


	++readyCount;
	while (readyCount < tCount)
	{
		std::chrono::milliseconds t(10);
		std::this_thread::sleep_for(t);
	}
	


	DataHeader login_out;
	
		login_out.datalength = sizeof(DataHeader);
		login_out.cmd = CMD::HEART_C;

	

	std::thread t1(RecvThread, &clients);
	t1.detach();

	CELLTimestamp tTime;
	int nlen = sizeof(login_out);
	while (g_Run)
	{
		for (auto it = clients.begin(); it != clients.end(); it++)
		{
			/*if (1.0 <= tTime.getElapsedSecond())
			{*/
			if (*it)
				(*it)->SendData((char*)&login_out, login_out.datalength);
				
			
			/*	tTime.update();
			}*/
		}
		std::chrono::milliseconds t(1);
		std::this_thread::sleep_for(t);
	}


	for (auto &x : clients)
	{
		if (x)
			delete x;
	}
	



}


int main()
{
	Log::Instance()->setLogPath("client.txt", "w");
	//tCount = GetPrivateProfileIntA("Client", "线程数量", 1, "./配置.ini");
	//sCount = GetPrivateProfileIntA("Client", "客户端数量", 10, "./配置.ini");
	//GetPrivateProfileStringA("Client", "IP", "127.0.0.1", IP, MAX_PATH, "./配置.ini");
	//WritePrivateProfileStringA("Client", "IP", "127.0.0.1", "./配置.ini");

	//std::thread t1(cmdThread);
	//t1.detach();//与父线程分离

	//std::vector<std::thread> threadarr;

	

	//for (int i = 0; i < tCount; i++)
	//{
	//	threadarr.push_back(std::thread(clientThread, i));
		//threadarr.back().detach();
	//}

	//CELLTimestamp tTime;
	
	//client.Connect("47.92.72.152", 8011);
	//char  http[] = "POST http://47.92.72.152:8011/session.ashx?action=login HTTP/1.1\n"
	//	"Host: 47.92.72.152:8011\n"
	//	"Connection: keep-alive\n"
	//	"Content-Length: 43\n"
	//	"Accept: */*\n"
	//	"Origin: http://47.92.72.152:8011\n"
	//	"X-Requested-With: XMLHttpRequest\n"
	//	"User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/79.0.3945.88 Safari/537.36\n"
	//	"Content-Type: application/x-www-form-urlencoded; charset=UTF-8\n"
	//	"Referer: http://47.92.72.152:8011/default.html\n"
	//	"Accept-Encoding: gzip, deflate\n"
	//	"Accept-Language: zh-CN,zh;q=0.9\n\n"
	//	"account=adminys&password=123456&code=8PFB0N";
	//hostent *_hostent = gethostbyname("29r3361k32.qicp.vip");
#if 1
	auto fu = []() {
		std::vector<MyClient *> clients;
		for (int i = 0; i < 100; i++)
		{
			auto x = new MyClient;
			if (x)
			{
				if (x->Connect("127.0.0.1", 4567))
				{
					char name[16] = {};
					CellMsgStream  send;
					sprintf(name, "test%d", i);
					send.writeString(name);
					send.writeString("123456");
					send.setNetCmd(CMD::LOGIN);
					x->SendData(send.getheader());
					clients.push_back(x);
				}
				else
				{
					delete x;
					printf("连接失败!");
				}

			}

		}
		//std::cout << "按任意键结束!" << std::endl;
		getchar();
		getchar();
		for (auto &x : clients)
		{
			if (x)
				delete x;
		}
	};
	for (unsigned int i = 0; i < std::thread::hardware_concurrency(); ++i)
	{
		std::thread t(fu);
		t.detach();
	}
	std::cout << "按任意键结束!" << std::endl;
	getchar();
	return 0;//192.168.2.222
	


#else	
		
			
	
	MyClient client;
	//hostent *_hostent = gethostbyname("29r3361k32.qicp.vip");
	//if (_hostent && _hostent->h_addr_list)
	//if (client.Connect(inet_ntoa(*((in_addr *)(*_hostent->h_addr_list))), 29208))
	if (client.Connect("127.0.0.1", 4567))
	{
		CellMsgStream  send;
			send.writeString("mailtest");
			send.writeString("zgr110064");
			send.setNetCmd(CMD::LOGIN);
			client.SendData(send.getheader());
			char c[32];
			while (1)
			{
				std::cin >> c;
					if (0 == strcmp("register", c))
					{
						send.setNetCmd(CMD::REGISTER);
						send.writeString("weiping19700914@163.com");
						client.SendData(send.getheader());
						std::cout << "已发送..." << std::endl;
					}
					else if (0 == strcmp("findpass", c))
					{
						printf("输入验证码:\n");
						std::cin >> c;
						send.setNetCmd(CMD::FIND_PASS);
						send.writeString(c);
						client.SendData(send.getheader());
						std::cout << "已发送..." << std::endl;
					}
					else if (0 == strcmp("getse", c))
					{
						send.setNetCmd(CMD::MAIL_SECURITY);
						client.SendData(send.getheader());
						std::cout << "已发送..." << std::endl;
					}
					else if (0 == strcmp("login", c))
					{
						send.setNetCmd(CMD::LOGIN);
						client.SendData(send.getheader());
						std::cout << "已发送..." << std::endl;
					}
					else if (0 == strcmp("logout", c))
					{
						send.setNetCmd(CMD::LOGOUT);
						client.SendData(send.getheader());
						std::cout << "已发送..." << std::endl;
					}
					else if (0 == strcmp("addtime", c))
					{
						send.setNetCmd(CMD::ADD_TIME);
						send.writeString("456asdads");
						client.SendData(send.getheader());
						std::cout << "已发送..." << std::endl;
					}
					else if (0 == strcmp("alter", c))
					{
						send.setNetCmd(CMD::ALTER_PASSWORD);
						send.writeString("zgr110064");
						client.SendData(send.getheader());
						std::cout << "已发送..." << std::endl;
					}
					else if (0 == strcmp("createcal", c))
					{
						send.setNetCmd(CMD::CREATE_CAL);
						send.write_int32(2);
						send.writeString("MON");
						send.write_int32(1);
						client.SendData(send.getheader());
						std::cout << "已发送..." << std::endl;
					}
					else if (0 == strcmp("exit", c))
					{
						client.Close();
						break;
					}

			}
	}
#endif	
	//while (g_Run)
	//{
	//	//auto t = tTime.getElapsedSecond();
	//	if (t >= 1.0)
	//	{
	//		printf("thread<%d>,time<%lf>,client<%d>,send<%d>\n",
	//			(int)tCount,
	//			t,
	//			(int)cCount,
	//			(int)(sendCount/t));
	//		sendCount = 0;
	//		tTime.update();
	//	}
	//	Sleep(1);
	//}

	//for (auto &x : threadarr)
	//{
	//	x.join();
	//}
	//std::cout << "程序已退出!\n";
	//std::thread t1(clientThread);
	//t1.detach();//与主线程分离

	//std::thread t2(clientThread);
	//t2.detach();//与主线程分离

	//std::thread t3(clientThread);
	//t3.detach();//与主线程分离

	//std::thread t4(clientThread);
	//t4.detach();//与主线程分离
	std::cout << "按任意键结束!" << std::endl;
	getchar();
	return 0;
}
