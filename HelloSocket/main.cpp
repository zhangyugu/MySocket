#define WIN32_LEAN_AND_MEAN
#include "CellMsgStream.hpp"
#include "EasyTcpClinet.hpp"

class MyClient : public EasyTcpClient
{
public:
	MyClient() {}
	~MyClient() {}
protected:
	virtual void OnNetMsg()
	{
		//Login_out *login_out = (Login_out *)header;
		DataHeader * front = pClinet->front();
		switch (front->cmd)
		{
		case CMD_HEART_S:
		{

		}
		break;
		case CMD_LOGIN_RESULT:
		{
			CellMsgStream s(front);
			int8_t status = s.read_int8();


			if (STATUS_SUCCEED == status)
				std::cout << "<socket = " << pClinet->sock << ">登录成功!" << std::endl;
			else
				std::cout << "<socket = " << pClinet->sock << ">登录失败!" << std::endl;
		}
		break;
		case CMD_LOGOUT_RESULT:
		{
			/*if (STATUS_SUCCEED == login_out->information)
				std::cout << "<socket = " << _sock << ">退出成功!" << std::endl;
			else
				std::cout << "<socket = " << _sock << ">退出失败!" << std::endl;*/
		}
		break;
		case CMD_NEWUSER_JOIN:
		{
			//std::cout << "新用户加入: socket = " << login_out->information << std::endl;
		}
		break;
		default:
			Log::Info("socket<%d> unknown message!", pClinet->sock);
			break;
		}

		EasyTcpClient::OnNetMsg();
	}
private:

};
//#pragma comment(lib, "ws2_32.lib")
int main()
{
	MyClient client;
	client.Connect("192.168.2.100", 4567);
	CellMsgStream s;
	s.setNetCmd(CMD_LOGIN);
	s.write_int8(1);
	s.write_int16(2);
	s.write_int32(3);
	s.write_int64(4);

	//s.write_array("463168397", 32);
	s.writeString("463168397");

	s.write_uint8(5);
	s.write_uint16(6);
	s.write_uint32(7);
	s.write_uint64(8);

	s.write_array("91011", 10);

	s.write_float(1.5);
	s.write_double(2.5);

	s.write_array("qq123456qq", 32);


	//char id[32], PassWord[32], tmp[10];
	//int8_t int8 = s.read_int8();
	//int16_t int16 = s.read_int16();
	//int32_t int32 = s.read_int32();
	//int64_t int64 = s.read_int64();

	//s.readCstring(id);
	////s.read_array(id, 32);
	//uint8_t  uint8 = s.read_uint8();
	//uint16_t uint16 = s.read_uint16();
	//uint32_t uint32 = s.read_uint32();
	//uint64_t uint64 = s.read_uint64();
	//s.read_array(tmp, 10);
	//float f = s.read_float();
	//double d = s.read_double();

	//s.read_array(PassWord, 32);
	//std::cout << "登录命令: " << id << "," << PassWord << " , " << tmp;
	//std::cout << std::endl;

	//return 0;

	client.SendData(s.getheader());
	/*while (1)
	{
		
		client.OnRunLoop();
	}*/






	//std::cout << d << std::endl;//
	//printf("%d-%d-%d-%lld-%u-%u-%u-%llu-%f-%lf-%s\n", int8, int16, int32, int64, uint8, uint16, uint32, uint64, f, d, str2);



	return 0;
}