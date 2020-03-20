#include "SockContext.h"
#include "EasyTcpServer.hpp"
#include"CellMsgStream.hpp"
#include "MyMySql.hpp"
#include "Send.h"


char  MAIL[] = "<!doctype html>\n"
"<html lang=\"en-US\">\n"
"<head>\n"
"	<meta charset=\"UTF-8\">\n"
"	<title></title>\n"
"</head>\n"
"<body>\n"
"<table width=\"700\" border=\"0\" align=\"center\" cellspacing=\"0\" style=\"width:700px;\">\n"
"	<tbody>\n"
"	<tr>\n"
"		<td>\n"
"			<div style=\"width:700px;margin:0 auto;border-bottom:1px solid #ccc;margin-bottom:30px;\">\n"
"				<table border=\"0\" cellpadding=\"0\" cellspacing=\"0\" width=\"700\" height=\"39\" style=\"font:12px Tahoma, Arial, 宋体;\">\n"
"					<tbody>\n"
"					<tr>\n"
"						<td width=\"210\">\n\n"
"						</td>\n"
"					</tr>\n"
"					</tbody>\n"
"				</table>\n"
"			</div>\n"
"			<div style=\"width:680px;padding:0 10px;margin:0 auto;\">\n"
"				<div style=\"line-height:1.5;font-size:14px;margin-bottom:25px;color:#4d4d4d;\">\n"
"					<strong style=\"display:block;margin-bottom:15px;\">\n"
"						亲爱的用户：\n"
"						<span style=\"color:#f60;font-size: 16px;\"></span>您好！\n"
"					</strong>\n\n"
"					<strong style=\"display:block;margin-bottom:15px;\">\n"
"						您正在重置密码，本次验证码5分钟内有效，输入验证码：\n"
"						<span style=\"color:#f60;font-size: 24px\">%s</span>，以完成操作。\n"//验证码在这
"					</strong>\n"
"				</div>\n\n"
"				<div style=\"margin-bottom:30px;\">\n"
"					<small style=\"display:block;margin-bottom:20px;font-size:12px;\">\n"
"						<p style=\"color:#747474;\">\n"
"							注意：此操作可能会修改您的密码。如非本人操作，请及时登录并修改密码以保证帐户安全\n"
"							<br>（工作人员不会向你索取此验证码，请勿泄漏！)\n"
"						</p>\n"
"					</small>\n"
"				</div>\n"
"			</div>\n"
"			<div style=\"width:700px;margin:0 auto;\">\n"
"				<div style=\"padding:10px 10px 0;border-top:1px solid #ccc;color:#747474;margin-bottom:20px;line-height:1.3em;font-size:12px;\">\n"
"					<p>此为系统邮件，请勿回复<br>\n"
"						请保管好您的邮箱，避免账号被他人盗用\n"
"					</p>\n"
"					<p>版权所有</p>\n"
"				</div>\n"
"			</div>\n"
"		</td>\n"
"	</tr>\n"
"	</tbody>\n"
"</table>\n"
"</body>\n"
"</html>\n";




class MyServer : public EasyTcpServer, public MyMySql
{
public:
	MyServer(unsigned short post, const char* addr = nullptr, unsigned short family = AF_INET) {
		//connect("localhost", "root", "110064", "userinfo");
		//query("insert into user(name, password) values('test1', 1234567)");
		//query("select * from serialnumber");
		//print();
		
		if (!CreateListenSocket(post, 1024))
			printf("bindport 失败!\n");
	}
	~MyServer(){}
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
			STATUS status = STATUS::FAILED;
			CellMsgStream stream(pClient->front());
			char id[32], PassWord[32];
			stream.read_array(id, 32);
			stream.read_array(PassWord, 32);
			do
			{
				std::lock_guard<std::mutex> lg(_mutex);
				char tmp[256];
				//查询是否存在
				sprintf(tmp, "select * from user where name='%s'", id);
				std::vector<std::vector<char*>> res = query(tmp);
				if (res.empty())
				{
					status = STATUS::NO_EXIST;//账号不存在
					break;
				}					
				if (0 != strcmp(res[0][1], PassWord))
				{
					status = STATUS::PASSWORD_ERROR;//密码错误STATUS::EXPIRE
					break;
				}
				std::tm tmptm = str2tm(res[0][2]);
				if (0 != strcmp(res[0][0], "root") && mktime(getSystemTime()) > mktime(&tmptm))
				{
					status = STATUS::EXPIRE;//到期
					break;
				}
				if (0 == strcmp(res[0][0], id))
				{
					status = STATUS::SUCCEED;
					memcpy(pClient->_id, id, sizeof(id));
					memcpy(pClient->_password, PassWord, sizeof(PassWord));
					if (0 == strcmp(res[0][0], "root"))
						pClient->_isadmin = true;
					pClient->SetLoginStatus(true);
					break;
				}
									
			} while (0);

			CellMsgStream  send;
			send.setNetCmd(CMD::STATUS);
			send.write_int16(static_cast<int16_t>(status));
			if (SOCKET_ERROR == pClient->SendData(send.getheader()))
			{
				//发送缓冲区满了
				printf("发送缓冲区满了\n");
			}
		}
		break;
		case CMD::LOGOUT:
		{
			
			CellMsgStream  send;
			send.setNetCmd(CMD::STATUS);
			send.write_int16(static_cast<int16_t>(STATUS::SUCCEED));
			if (SOCKET_ERROR == pClient->SendData(send.getheader()))
			{
				//发送缓冲区满了
				printf("发送缓冲区满了\n");
			}
			//std::cout << "客户端退出" << std::endl;
			pClient->SetLoginStatus(false);
			////接收包数据
			//Login_out * login_out = (Login_out *)pheader;
			//std::cout << "登录命令: " << login_out->userName << "," << login_out->PassWord << std::endl;
			////不验证账号密码  直接发送成功状态
			//login_out->cmd = CMD::LOGOUT_RESULT;
			//login_out->datalength = sizeof(Login_out);
			//login_out->information = STATUS::SUCCEED;
			//SendData(&pinfo->sock, login_out);
		}
		break;
		case CMD::REGISTER:
		{
			STATUS status = STATUS::EXIST;
			CellMsgStream stream(pClient->front());
			char id[32], PassWord[32], mailbox[32];
			stream.read_array(id, 32);
			stream.read_array(PassWord, 32);
			stream.read_array(mailbox, 32);
			{
				std::lock_guard<std::mutex> lg(_mutex);
				char tmp[256];
				//查询是否存在
				sprintf(tmp, "select * from user where name='%s'", id);
				query(tmp);
				if (0 == affected() && 0 != strcmp(id, "root"))
				{
					//如果不存在则添加
					sprintf(tmp, "insert into user(name, password, mailbox) values('%s', '%s', '%s')", id, PassWord, mailbox);
					query(tmp);
					if (1 == affected())
						status = STATUS::SUCCEED;
					else
						status = STATUS::FAILED;
				}
			}

			CellMsgStream  send;
			send.setNetCmd(CMD::STATUS);
			send.write_int16(static_cast<int16_t>(status));
			if (SOCKET_ERROR == pClient->SendData(send.getheader()))
			{
				//发送缓冲区满了
				printf("发送缓冲区满了\n");
			}
		}
		break;
		case CMD::MAIL_SECURITY:
		{
			STATUS status = STATUS::FAILED;
			CellMsgStream stream(pClient->front()), send;
			char id[32], PassWord[32];
			stream.read_array(id, 32);
			stream.read_array(PassWord, 32);
			{
				do
				{
					std::lock_guard<std::mutex> lg(_mutex);
					char tmp[256];
					//查询是否存在
					sprintf(tmp, "select * from user where name='%s'", id);
					std::vector<std::vector<char*>> res = query(tmp);
					if (res.empty())
					{
						status = STATUS::NO_EXIST;//账号不存在
						break;
					}
					if (0 != strcmp(res[0][1], PassWord))
					{
						status = STATUS::PASSWORD_ERROR;//密码错误STATUS::EXPIRE
						break;
					}					
					if (0 == strcmp(res[0][0], id))
					{
						auto now = CELLTime::getNowinMilliSec();
						if ((pClient->_dtsecurity == 0 || now - pClient->_dtsecurity > 1000 * 60) && strlen(res[0][3]) > 0)
						{
							std::string security = getSecurity();
							char buf[2000] = {};
							sprintf(buf, MAIL, security.c_str());
							SendMail sendmail;
							if (sendmail.BeginSendMail("zgr110064@163.com", "zgr110064", "玉谷科技", res[0][3], "用户邮箱重置密码验证", buf))
							{
								status = STATUS::SUCCEED;
								pClient->_dtsecurity = now;
								pClient->security = security;
							}
						}
						else
							status = STATUS::SECURITY_FRE;
						
						break;
					}

				} while (0);
				
			}

			send.setNetCmd(CMD::STATUS);
			send.write_int16(static_cast<int16_t>(status));
			if (SOCKET_ERROR == pClient->SendData(send.getheader()))
			{
				//发送缓冲区满了
				printf("发送缓冲区满了\n");
			}
		}
		break;
		case CMD::FIND_PASS:
		{
			STATUS status = STATUS::FAILED;
			CellMsgStream stream(pClient->front()), send;
			char id[32], PassWord[32], security[32];
			stream.read_array(id, 32);
			stream.read_array(PassWord, 32);
			stream.read_array(security, 32);
			{
				do
				{
					std::lock_guard<std::mutex> lg(_mutex);
					char tmp[256];
					//查询是否存在
					sprintf(tmp, "select * from user where name='%s'", id);
					std::vector<std::vector<char*>> res = query(tmp);
					if (res.empty())
					{
						status = STATUS::NO_EXIST;//账号不存在
						break;
					}
					if (0 == strcmp(res[0][0], id))
					{
						//STATUE_SECURITY_CARD
						auto now = CELLTime::getNowinMilliSec();
						if (now - pClient->_dtsecurity < 1000 * 60 * 5)
						{							
							if (pClient->security == security)
							{

								sprintf(tmp, "update user set password=%s where name='%s'", PassWord, id);
								query(tmp);
								if (affected())
								{
									status = STATUS::SUCCEED;
									pClient->_dtsecurity = 0;
									pClient->security.clear();
								}	
							}
							else
								status = STATUS::SECURITY_ERROR;
						}
						else
							status = STATUS::SECURITY_CARD;

						break;
					}

				} while (0);

			}

			send.setNetCmd(CMD::STATUS);
			send.write_int16(static_cast<int16_t>(status));
			if (SOCKET_ERROR == pClient->SendData(send.getheader()))
			{
				//发送缓冲区满了
				printf("发送缓冲区满了\n");
			}
		}
		break;
		case CMD::ADD_TIME:
		{
			STATUS status = STATUS::NO_EXIST;
			CellMsgStream stream(pClient->front());
			char id[32], PassWord[32], serialnumber[32];
			stream.read_array(id, 32);
			stream.read_array(PassWord, 32);
			stream.read_array(serialnumber, 32);
			do
			{
				std::lock_guard<std::mutex> lg(_mutex);
				char tmp[256];
				//查询是否存在
				sprintf(tmp, "select * from user where name='%s'", id);
				std::vector<std::vector<char*>> res = query(tmp);
				sprintf(tmp, "select * from serialnumber where serial='%s'", serialnumber);
				std::vector<std::vector<char*>> resserial = query(tmp);

				if (res.empty())
				{
					status = STATUS::NO_EXIST;//账号不存在
					break;
				}
				if (resserial.empty() || 0 == strcmp(resserial[0][2], "true"))
				{
					status = STATUS::NO_CARD;//充值卡不存在
					break;
				}
				if (0 != strcmp(res[0][1], PassWord))
				{
					status = STATUS::PASSWORD_ERROR;//密码错误STATUS::EXPIRE
					break;
				}			
				if (0 == strcmp(res[0][0], id))
				{
					status = STATUS::SUCCEED;
					char chartmp[64] = {}, charnow[32];
					int day = str2int(resserial[0][1]);
					time_t now = mktime(getSystemTime(charnow));
					std::tm tmptm = str2tm(res[0][2]);
					time_t usertime = mktime(&tmptm);
					time_t tmp_t;
					if (now > usertime)	 tmp_t = now + day * 86400;//如果小于当前时间
					else  tmp_t = usertime + day * 86400;
					tm2str(*std::localtime(&tmp_t), chartmp);

					sprintf(tmp, "update user set time='%s' where name='%s'", chartmp, id);//更新用户时间
					query(tmp);
					if (1 != affected())
					{
						status = STATUS::FAILED;//未知原因失败
						break;
					}
					

					sprintf(tmp, "update serialnumber set enable='true',enableTime='%s' where serial='%s'", charnow, resserial[0][0]);//更新充值卡
					query(tmp);

					//pClient->SetLoginStatus(true);
					break;
				}

			} while (0);

			CellMsgStream  send;
			send.setNetCmd(CMD::STATUS);
			send.write_int16(static_cast<int16_t>(status));
			if (SOCKET_ERROR == pClient->SendData(send.getheader()))
			{
				//发送缓冲区满了
				printf("发送缓冲区满了\n");
			}
		}
		break;
		case CMD::ALTER_PASSWORD:
		{

			STATUS status = STATUS::FAILED;
			CellMsgStream stream(pClient->front());
			char id[32], PassWord[32], new_password[32];
			stream.read_array(id, 32);
			stream.read_array(PassWord, 32);
			stream.read_array(new_password, 32);
			do
			{
				std::lock_guard<std::mutex> lg(_mutex);
				char tmp[256];
				//查询是否存在
				sprintf(tmp, "select * from user where name='%s'", id);
				std::vector<std::vector<char*>> res = query(tmp);
				if (res.empty())
				{
					status = STATUS::NO_EXIST;//账号不存在
					break;
				}
				if (0 != strcmp(res[0][1], PassWord))
				{
					status = STATUS::PASSWORD_ERROR;//密码错误STATUS::EXPIRE
					break;
				}
				if (0 == strcmp(res[0][0], id))
				{

					sprintf(tmp, "update user set password='%s' where name='%s'", new_password, id);
					query(tmp);
					if (1 == affected())
					{
						status = STATUS::SUCCEED;//未知原因失败
						break;
					}				
				}

			} while (0);

			CellMsgStream  send;
			send.setNetCmd(CMD::STATUS);
			send.write_int16(static_cast<int16_t>(status));
			if (SOCKET_ERROR == pClient->SendData(send.getheader()))
			{
				//发送缓冲区满了
				printf("发送缓冲区满了\n");
			}
		}
		break;
		case CMD::CREATE_CAL:
		{
			if (pClient->_isadmin)
			{
				STATUS status = STATUS::FAILED;				
				char id[32], PassWord[32], calhead[32];
				int num, day;
				CellMsgStream stream(pClient->front()), send;
				stream.read_array(id, 32);
				stream.read_array(PassWord, 32);
				num = stream.read_int32();//数量
				stream.read_array(calhead, 32);//头
				day = stream.read_int32();//天数
				do
				{
					std::lock_guard<std::mutex> lg(_mutex);
					//char tmp[256];
					//查询是否存在
					status = checkuser(id, PassWord);
					if (STATUS::SUCCEED == status)
					{
						std::vector<std::string> arr = CreateSerialNumber(num, calhead, day);
						if (arr.empty())
						{
							status = STATUS::FAILED;
							send.write_int16(static_cast<int16_t>(status));
						}
						else
						{
							status = STATUS::SUCCEED_CREATE_CAL;
							send.write_int16(static_cast<int16_t>(status));//状态码
							send.write_int32((int)arr.size());//数量
							for (auto &x : arr)
							{
								send.writeString(x);
								//std::cout << x.c_str() << std::endl;
							}
						}

					}
				} while (0);

	
				send.setNetCmd(CMD::STATUS);					
				if (SOCKET_ERROR == pClient->SendData(send.getheader()))
				{
					//发送缓冲区满了
					printf("发送缓冲区满了\n");
				}
			}
			
		}
		break;
		default:
		{
			std::cout << "未知命令 " << std::endl;
		}
		break;
		}

		EasyTcpServer::OnNetMsg(pCellServer, pClient);
	}

	std::vector<std::string> CreateSerialNumber(int n = 1, const char * head = "MON", int day = 31)
	{
		std::vector<std::string> vecret;
		char serial[33] = {};
		int seriallen = 0;
		int headlen = (int)strlen(head);
		for (int i = 0; i < n; )
		{
			seriallen = 0;
			for (int n = 0; n < headlen; ++n)
				serial[seriallen++] = head[n];
			for (int m = 0, ser = seriallen; m < 32 - ser && seriallen < 32; ++m, ++seriallen)
			{
				if (rand(0, 1))
					serial[seriallen] = (char)rand(65, 90);
				else
					serial[seriallen] = (char)rand(97, 122);
			}
			if (seriallen == 32)
			{
				serial[seriallen] = '\0';
				char tmp[128] = {}, timenow[64] = {};
				getSystemTime(timenow);
				sprintf(tmp, "insert serialnumber(serial,time,createTime) values('%s',%d,'%s')", serial, day, timenow);
				query(tmp);
				if (1 == affected())				
					vecret.push_back(std::string(serial));
				else
				{
					printf_error();
					continue;
				}
					
			}
			++i;
		}

		return vecret;
	}
	inline STATUS checkuser(const char *id, const char *password, std::vector<std::vector<char*>> *pres = nullptr)
	{
		char tmp[256];
		std::vector<std::vector<char*>> res;
		//查询是否存在
		if (nullptr == pres)
		{
			sprintf(tmp, "select * from user where name='%s'", id);
			res = query(tmp);
		}
		else
			res = *pres;

		if (res.empty())
			return STATUS::NO_EXIST;//账号不存在				
		if (0 != strcmp(res[0][1], password))	
			return STATUS::PASSWORD_ERROR;//密码错误STATUS::EXPIRE		
		if (0 == strcmp(res[0][0], id))	
			return STATUS::SUCCEED;
		return STATUS::FAILED;
	}
	std::string getSecurity()
	{
		std::string restr;
		char tmp[] = "0123456789";
		for (int i = 0; i < 6; ++i)
			restr.push_back(tmp[rand(0, 9)]);
		return restr;
	}
private:

};

std::string getSecurity()
{
	std::string restr;
	char tmp[] = "0123456789";
	for (int i = 0; i < 6; ++i)
		restr.push_back(tmp[rand(0, 9)]);
	return restr;
}

#include <string>
int main(void)
{

	//计算时间差(秒)
	//double seconds = difftime(mktime(getSystemTime(now)), mktime(&str2tm("2020-01-15 19:00:00")));
	MyServer server(4567);							 //userinfo
	if (!server.connect("localhost", "root", "110064", "userinfo")) 
	{
		std::cout << "数据库连接失败!" << std::endl;
		return 1;
	}
	/*for (int i = 0; i < 10000; ++i)
	{
		char tmp[128] = {};
		sprintf(tmp, "insert into user(name, password, time) values('test%d', '123456', '2020-04-01 00:00:00')", i);
		server.query(tmp);
	}*/
	
	//server.query("select * from serialnumber");
	//server.print();
	server.Start();
	
	

	//std::cout << std::endl;
	getchar();
	return 0;
}
