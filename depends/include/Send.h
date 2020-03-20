#pragma once
#ifndef  __INCLUDE_SEND_MAIL_H__
#define  __INCLUDE_SEND_MAIL_H__


#include "vector"
using namespace std;

#include "SockContext.h"
#include <fstream>
//#include "atlenc.h"



struct sMailInfo					//邮件信息
{
	char *mailbox_user_name;		//用户登录邮箱的名称
	char *mailbox_user_pwd;			//用户登录邮箱的password
	char *mailbox_sender_show_name;	//用户发送时显示的名称
//	char *mailbox_sender;			//发送者的邮箱地址
	char *mailbox_receiver;			//接收者的邮箱地址
	char *mail_title;				//邮箱标题
	char *mail_body;				//邮件文本正文
	char mail_server_ip_addr[32];	//服务器的IP
//	char *mail_server_name;			//服务器的名称（IP与名称二选一，优先取名称）
	sMailInfo() { memset(this, 0, sizeof(sMailInfo)); }
	sMailInfo(char* username, char *userpwd, char *sendername, char *receiver, char *title, char *body) :
		mailbox_user_name(username), mailbox_user_pwd(userpwd), mailbox_sender_show_name(sendername),
		mailbox_receiver(receiver), mail_title(title), mail_body(body) {}
};


class SendMail
{
public:
	SendMail()
	{
		file_buffer_ = NULL;
		memset(send_buff_, 0, sizeof(send_buff_));
		memset(receive_buff_, 0, sizeof(receive_buff_));
	}
	~SendMail()
	{
		DeleteAllPath();
	}

public:
	bool BeginSendMail(sMailInfo &smail_info)	//发送邮件，须要在发送的时候初始化邮件信息
	{
		memcpy(&mail_info_, &smail_info, sizeof(smail_info));
		if (mail_info_.mail_body == NULL
			|| mail_info_.mail_server_ip_addr == NULL
			|| mail_info_.mailbox_receiver == NULL
			|| mail_info_.mailbox_sender_show_name == NULL
			|| mail_info_.mail_title == NULL
			|| mail_info_.mailbox_user_name == NULL
			|| mail_info_.mailbox_user_pwd == NULL)
		{
			return false;
		}

		SOCKET sock;
		if (!CReateSocket(sock)) // 建立连接
		{
			return false;
		}

		if (!Logon(sock))		// 登录邮箱
		{
			return false;
		}

		if (!SendHead(sock))		// 发送邮件头
		{
			return false;
		}

		if (!SendTextBody(sock)) // 发送邮件文本部分
		{
			return false;
		}

		//if (!SendFileBody(sock)) // 发送附件
		//{
		//	return false;
		//}

		if (!SendEnd(sock))		// 结束邮件，并关闭sock
		{
			return false;
		}

		return true;
	}
	bool BeginSendMail(const char * username, const char * userpwd, const char * sendername, const char * receiver, const char * title, const char * body)
	{

		sMailInfo info((char*)username, (char*)userpwd, (char*)sendername, (char*)receiver, (char*)title, (char*)body);
		return BeginSendMail(info);
	}
	void AddFilePath(char *file_path)			//加入附件的决定路径到附件列表中
	{
		if (file_path == NULL)
		{
			return;
		}
		char *temp;
		for (int index = 0; index < file_path_list_.size(); index++)
		{
			temp = file_path_list_[index];
			if (strcmp(file_path, temp) == 0)			// 假设已经存在就不用再加入了
			{
				return;
			}
		}
		file_path_list_.push_back(file_path);
	}
	void DeleteFilePath(char *file_path)		//删除附件路径，假设有的话
	{
		char* temp;
		for (int index = 0; index < file_path_list_.size(); index++)
		{
			temp = file_path_list_[index];
			if (strcmp(file_path, temp) == 0)
			{
				file_path_list_.erase(file_path_list_.begin() + index);
			}
		}
	}
	void DeleteAllPath(void)					//删除所有附件的路径
	{
		vector<char *>().swap(file_path_list_);		// 清除容器并最小化它的容量
	}

protected:
	void GetFileName(char *file_name, char *file_path)	//从附件的路径中获取文件名
	{
		if (file_path == NULL || file_name == NULL)
		{
			return;
		}
		for (int index = 0; index < (int)strlen(file_path); index++)
		{
			if (file_path[strlen(file_path) - 1 - index] == '\\')
			{
				memcpy(file_name, &file_path[strlen(file_path) - index], index);
				return;
			}
		}
	}
	void Char2Base64(char *buff_64, char *src_buff, int length)//把char类型转换成Base64类型
	{
		//1   1   1   1   1   1   1   1
	// 分配给pBuff64  ↑ 分配给pBuff64+1
	// point所在的位置
		static char base_64_encode[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";//base64所映射的字符表
		int point;		// 每个源字符拆分的位置，可取2,4,6。初始为2
		point = 2;
		int base_index; // base64字符的索引
		char n = 0;		// 上一个源字符的残留值
		for (int index = 0; index < length; index++)
		{
			if (point == 2)
			{
				base_index = ((*src_buff) >> point) & 0x3f; // 取得pSrcBuff的高point位
			}
			else if (point == 4)
			{
				base_index = ((*src_buff) >> point) & 0xf;	// 取得pSrcBuff的高point位
			}
			else if (point == 6)
			{
				base_index = ((*src_buff) >> point) & 0x3;	// 取得pSrcBuff的高point位
			}
			base_index += n;								// 与pSrcBuff-1的低point结合组成Base64的索引
			*buff_64++ = base_64_encode[base_index];		// 由索引表得到pBuff64
			n = ((*src_buff) << (6 - point));				// 计算源字符中的残留值
			n = n & 0x3f;									//确保n的最高两位为0
			point += 2;										//源字符的拆分位置上升2
			if (point == 8)		//假设拆分位置为8说明pSrcBuff有6位残留。能够组成一个完整的Base64字符。所以直接再组合一次
			{
				base_index = (*src_buff) & 0x3f;			//提取低6位。这就是索引了
				*buff_64++ = base_64_encode[base_index];
				n = 0;			// 残留值为0
				point = 2;		// 拆分位置设为2
			}
			src_buff++;

		}
		if (n != 0)
		{
			*buff_64++ = base_64_encode[n];
		}
		if (length % 3 == 2)	// 假设源字符串长度不是3的倍数要用'='补全
		{
			*buff_64 = '=';
		}
		else if (length % 3 == 1)
		{
			*buff_64++ = '=';
			*buff_64 = '=';
		}
	}
	bool CReateSocket(SOCKET &sock)			//建立socket连接
	{
		sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
		if (sock == INVALID_SOCKET)
		{
			return false;
		}

		sockaddr_in server_addr;
		memset(&server_addr, 0, sizeof(sockaddr_in));
		server_addr.sin_family = AF_INET;
		server_addr.sin_port = htons(25);	// 发邮件一般都是25port，SSL的是465port


		std::string tmp(mail_info_.mailbox_user_name), server;
		for (size_t i = tmp.find("@") + 1; i < tmp.size(); ++i)
			server.push_back(mail_info_.mailbox_user_name[i]);

		std::string ip;
		hostent *_hostent = gethostbyname(("smtp." + server).c_str());
		if (_hostent && _hostent->h_addr_list)
			ip  = inet_ntoa(*((in_addr *)(*_hostent->h_addr_list)));
 		// = getip();//inet_ntoa(server_addr.sin_addr);ip = "220.181.12.14"
		if (ip.empty())
			return false;

		strcpy(mail_info_.mail_server_ip_addr, ip.c_str());

#ifdef _WIN32
		server_addr.sin_addr.S_un.S_addr = inet_addr(ip.c_str());//绑定本机ip地址  INADDR_ANY 本机的全部地址  //inet_addr("127.0.0.1");
#else
		server_addr.sin_addr.s_addr = inet_addr(ip.c_str());
#endif
		int ret = connect(sock, (sockaddr*)&server_addr, sizeof(sockaddr));	// 建立连接
		if (ret == SOCKET_ERROR)
		{
			return false;
		}

		return true;
	}
	bool Logon(SOCKET &sock)					//登录邮箱。主要进行发邮件前的准备工作
	{
		recv(sock, receive_buff_, 1024, 0);

		memset(send_buff_, 0, sizeof(send_buff_));
		sprintf(send_buff_, "HELO %s\r\n", mail_info_.mail_server_ip_addr);
		send(sock, send_buff_, (int)strlen(send_buff_), 0);	// 開始会话
		memset(receive_buff_, 0, sizeof(receive_buff_));
		recv(sock, receive_buff_, 1024, 0);
		if (receive_buff_[0] != '2' || receive_buff_[1] != '5' || receive_buff_[2] != '0')
		{
			printf("%s\n", receive_buff_);
			return false;
		}

		memset(send_buff_, 0, sizeof(send_buff_));
		sprintf(send_buff_, "AUTH LOGIN\r\n");
		send(sock, send_buff_, (int)strlen(send_buff_), 0);	// 请求登录
		recv(sock, receive_buff_, 1024, 0);
		if (receive_buff_[0] != '3' || receive_buff_[1] != '3' || receive_buff_[2] != '4')
		{
			printf("%s\n", receive_buff_);
			return false;
		}

		memset(send_buff_, 0, sizeof(send_buff_));
		Char2Base64(send_buff_, mail_info_.mailbox_user_name, (int)strlen(mail_info_.mailbox_user_name));
		send_buff_[strlen(send_buff_)] = '\r';
		send_buff_[strlen(send_buff_)] = '\n';
		send(sock, send_buff_, (int)strlen(send_buff_), 0);	// 发送username
		recv(sock, receive_buff_, 1024, 0);
		if (receive_buff_[0] != '3' || receive_buff_[1] != '3' || receive_buff_[2] != '4')
		{
			printf("%s\n", receive_buff_);
			return false;
		}

		memset(send_buff_, 0, sizeof(send_buff_));
		Char2Base64(send_buff_, mail_info_.mailbox_user_pwd, (int)strlen(mail_info_.mailbox_user_pwd));
		send_buff_[strlen(send_buff_)] = '\r';
		send_buff_[strlen(send_buff_)] = '\n';
		send(sock, send_buff_, (int)strlen(send_buff_), 0);	// 发送用户password
		recv(sock, receive_buff_, 1024, 0);
		if (receive_buff_[0] != '2' || receive_buff_[1] != '3' || receive_buff_[2] != '5')
		{
			printf("%s\n", receive_buff_);
			return false;
		}
		return true;	// 登录成功
	}
	int GetFileData(char *file_path)			//由文件路径获取附件内容
	{
		file_buffer_ = NULL;
		if (file_path == NULL)
		{
			return 0;
		}

		// 通过读取文件的大小，来开辟数组空间的大小
		ifstream fin(file_path);
		if (!fin.is_open())
		{
			return -1;
		}
		fin.seekg(0, ios::end);
		int file_length = (int)fin.tellg();
		fin.seekg(0, ios::beg);
		file_buffer_ = new char[file_length];
		fin.read(file_buffer_, file_length);
		fin.close();

		return file_length;
	}

	bool SendHead(SOCKET &sock)				//发送邮件头
	{
		int rt;
		memset(send_buff_, 0, sizeof(send_buff_));
		sprintf(send_buff_, "MAIL FROM:<%s>\r\n", mail_info_.mailbox_user_name);
		rt = send(sock, send_buff_, (int)strlen(send_buff_), 0);

		if (rt != strlen(send_buff_))
		{
			printf("send <%s> failed\n", send_buff_);
			return false;
		}
		if (recv(sock, receive_buff_, 1024, 0) > 0)
		{
			if (receive_buff_[0] != '2' || receive_buff_[1] != '5' || receive_buff_[2] != '0')
			{
				printf("<%s>\n", receive_buff_);
				return false;

			}
		}


		memset(send_buff_, 0, sizeof(send_buff_));
		sprintf(send_buff_, "RCPT TO:<%s>\r\n", mail_info_.mailbox_receiver);
		rt = send(sock, send_buff_, (int)strlen(send_buff_), 0);
		if (rt != strlen(send_buff_))
		{
			printf("send <%s> failed\n", send_buff_);
			return false;
		}
		if (recv(sock, receive_buff_, 1024, 0) > 0)
		{
			if (receive_buff_[0] != '2' || receive_buff_[1] != '5' || receive_buff_[2] != '0')
			{
				printf("<%s>\n", receive_buff_);
				return false;

			}
		}

		memset(send_buff_, 0, sizeof(send_buff_));
		memcpy(send_buff_, "DATA\r\n", strlen("DATA\r\n"));
		rt = send(sock, send_buff_, (int)strlen(send_buff_), 0);
		if (rt != strlen(send_buff_))
		{
			printf("send <%s> failed\n", send_buff_);
			return false;
		}
		if (recv(sock, receive_buff_, 1024, 0) > 0)
		{
			if (receive_buff_[0] != '3' || receive_buff_[1] != '5' || receive_buff_[2] != '4')
			{
				printf("<%s>\n", receive_buff_);
				return false;

			}
		}

		memset(send_buff_, 0, sizeof(send_buff_));
		sprintf(send_buff_, "From:\"%s\"<%s>\r\n", mail_info_.mailbox_sender_show_name, mail_info_.mailbox_user_name);
		sprintf(&send_buff_[strlen(send_buff_)], "To:\"INVT.COM.CN\"<%s>\r\n", mail_info_.mailbox_receiver);
		sprintf(&send_buff_[strlen(send_buff_)], "Subject:%s\r\nMime-Version: 1.0\r\nContent-Type: text/html; boundary=\"INVT\"\r\n\r\n", mail_info_.mail_title);
		rt = send(sock, send_buff_, (int)strlen(send_buff_), 0);
		if (rt != strlen(send_buff_))
		{
			printf("send <%s> failed\n", send_buff_);
			return false;
		}

		return true;
	}
	bool SendTextBody(SOCKET &sock)			//发送邮件文本正文
	{
		int rt;
		memset(send_buff_, 0, sizeof(send_buff_));
		sprintf(send_buff_, "%s\r\n\r\n", mail_info_.mail_body);//--INVT\r\nContent-Type: text/plain;\r\n  charset=\"gb2312\"\r\n\r\n
		rt = send(sock, send_buff_, (int)strlen(send_buff_), 0);
		if (rt != strlen(send_buff_))
		{
			printf("send <%s> failed\n", send_buff_);
			return false;
		}
		else
		{
			return true;
		}
	}
	//bool SendFileBody(SOCKET &sock)			//发送邮件附件
	//{
//	char* file_path;
//	int rt;
//	int len;
//	char file_name[128];
//	int pt = 0;
//	int dest_length = 0;
//
//	for (int index = 0; index < file_path_list_.size(); index++)
//	{
//		pt = 0;
//		memset(file_name, 0, 128);
//		file_path = file_path_list_[index];
//		len = GetFileData(file_path);
//		GetFileName(file_name, file_path);
//
//		sprintf(send_buff_, "--INVT\r\nContent-Type: application/octet-stream;\r\n  name=\"%s\"\r\nContent-Transfer-Encoding: base64\r\nContent-Disposition: attachment;\r\n  filename=\"%s\"\r\n\r\n", file_name, file_name);
//		send(sock, send_buff_, strlen(send_buff_), 0);
//		while (pt < len)
//		{
//			memset(send_buff_, 0, sizeof(send_buff_));
//
//			// 760为1024的最大整数,(760/4)*3=570
//			dest_length = 4096;
//			Base64Encode((BYTE*)file_buffer_ + pt, min(len - pt, 570), send_buff_, &dest_length);
//			int len1 = strlen(send_buff_);
//
//			pt += min(len - pt, 570);
//			if (pt == len)
//			{
//				send_buff_[len1] = '\r';
//				send_buff_[len1 + 1] = '\n';
//				send_buff_[len1 + 2] = '\0';
//			}
//			else
//				send_buff_[len1] = '\0';
//
//			rt = send(sock, send_buff_, strlen(send_buff_), 0);
//			if (rt != strlen(send_buff_))
//			{
//				return false;
//			}
//		}
//		if (len != 0)
//		{
//			delete[] file_buffer_;
//		}
//	}
//
//	return true;
//}
	bool SendEnd(SOCKET &sock)					//发送邮件结尾
	{
		sprintf(send_buff_, "\r\n.\r\n");//--INVT--
		send(sock, send_buff_, (int)strlen(send_buff_), 0);
		if (recv(sock, receive_buff_, 1024, 0) > 0)
		{
			if (receive_buff_[0] != '2' || receive_buff_[1] != '5' || receive_buff_[2] != '0')
			{
				printf("<%s>\n", receive_buff_);
				return false;

			}
		}

		sprintf(send_buff_, "QUIT\r\n");
		send(sock, send_buff_, (int)strlen(send_buff_), 0);
		if (recv(sock, receive_buff_, 1024, 0) > 0)
		{
			if (receive_buff_[0] != '2' || receive_buff_[1] != '2' || receive_buff_[2] != '1')
			{
				printf("<%s>\n", receive_buff_);
				return false;

			}
		}
#ifdef _WIN32
		closesocket(sock);
#else
		close(sock);
#endif
		return true;
	}

protected:
	vector<char *> file_path_list_;				//记录附件路径
	char  send_buff_[4096];						//发送缓冲区
	char  receive_buff_[1024];
	char *file_buffer_;							// 指向附件的内容
	sMailInfo mail_info_;
};


#endif