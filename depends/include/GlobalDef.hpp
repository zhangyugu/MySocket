#pragma once
#ifndef GLOBAL_DEF_HPP
#define GLOBAL_DEF_HPP

#define MAX_CLIENT 5000		//单线程处理客户端数量
#define _BASE64				//使用base64加密传输

#define CLIENT_HREAT_DEAD_TIME 60000 //心跳检测死亡计时时间(毫秒)
#define RECV_BUFF_SIZE 8192//10240//缓冲区大小
#define SEND_BUFF_SIZE RECV_BUFF_SIZE
#include "SockContext.h"
#include "CELLTimestamp.hpp"//时钟
#include "Log.hpp"


#include <iostream>//std::IO流
#include <vector>//std::向量
#include <list>//std::链表
#include <queue>//std::队列
#include <map>//std::映射
#include <thread>//std::线程
#include <mutex>//std::锁
#include <atomic>//std::原子操作
#include <functional>//泛函数 std::mum_fn
#include <algorithm>//std::算法
#include <assert.h>//断言
#include <memory>//std::内存
#include <ctime>//C时间
#include <random>//随机数


#ifdef _DEBUG
	#ifndef xPrintf
		#define xPrintf(...) printf(__VA_ARGS__)
	#endif
#else
	#ifndef xPrintf
		#define xPrintf(...)
	#endif
#endif//_DEBUG

enum class CMD : short
{
	LOGIN,
	LOGIN_RESULT,
	LOGOUT,
	LOGOUT_RESULT,
	NEWUSER_JOIN,
	HEART_S,
	HEART_C,
	REGISTER,
	STATUS,
	ADD_TIME,				//充值
	ALTER_PASSWORD,			//修改密码
	CREATE_CAL,				//创建充值卡
	FIND_PASS,				//找回密码
	MAIL_SECURITY,			//获取验证码
};
enum class STATUS : short
{
	SUCCEED,				//成功
	FAILED,				//失败
	PASSWORD_ERROR,		//密码错误
	EXIST,				//账号已存在
	NO_EXIST,			//账号不存在
	EXPIRE,				//账号到期
	NO_CARD,				//充值卡不存在
	SUCCEED_CREATE_CAL,	//成功创建充值卡
	SECURITY_CARD,		//验证码已过期
	SECURITY_ERROR,		//验证码错误
	SECURITY_FRE			//验证码获取频繁

	
};
//包头
struct DataHeader
{
	unsigned short datalength;//数据长度
	CMD cmd;//命令
	//char a[96];
};
//数据
struct Login_out : public DataHeader
{
	Login_out() : DataHeader{} {}
	char userName[32];//账号
	char PassWord[32];//密码
	int information;//附加信息
	//char test[24];
	//char test[924];

};


	


#ifndef GLOBAL_FUNCTION
#define GLOBAL_FUNCTION

template <typename T>
T power(T x, int n)
{
	if (0 == n)
		return 1;
	T tmp = x;
	for (int i = 0; i < n - 1; ++i)
		x *= tmp;
	return x;
}
int str2int(const char * str)
{
	const int strlength = (int)strlen(str);
	int retn = 0, retlen = 0;
	char chartmp[16] = {};
	for (int i = 0; i < strlength && retlen < 16; ++i)
	{
		if (str[i] >= 0x30 && str[i] <= 0x39)
			chartmp[retlen++] = str[i];
	}
	for (int i = 0, j = retlen - 1; i < retlen && j >= 0; ++i, --j)
	{
		retn += (chartmp[i] ^ 0x30) * power(10, j);
	}
	return retn;
}

std::tm str2tm(const char * str)
{
	const int strlength = (int)strlen(str);
	char chartmp[16] = {};
	int tmplen = 0, timelen = 0;
	std::tm tm = {};
	for (int i = 0; i <= strlength; ++i)
	{
		if (i != strlength && str[i] >= 0x30 && str[i] <= 0x39 && tmplen < 15)
			chartmp[tmplen++] = str[i];
		else
		{		
			chartmp[tmplen] = '\0';
			tmplen = 0;
			switch (timelen++)
			{
			case 0:
				tm.tm_year = str2int(chartmp) - 1900;
				break;
			case 1:
				tm.tm_mon = str2int(chartmp) - 1;
				break;
			case 2:
				tm.tm_mday = str2int(chartmp);
				break;
			case 3:
				tm.tm_hour = str2int(chartmp);
				break;
			case 4:
				tm.tm_min = str2int(chartmp);
				break;
			case 5:
				tm.tm_sec = str2int(chartmp);
				break;
			default:
				break;
			}
		}
	}
	return tm;
}

void tm2str(std::tm &tm, char * str)
{
	sprintf(str, "%d-%02d-%02d %02d:%02d:%02d",	tm.tm_year + 1900,	//年
										tm.tm_mon + 1,		//月
										tm.tm_mday,			//日
										tm.tm_hour,			//时
										tm.tm_min,			//分
										tm.tm_sec);			//秒
}

std::tm * getSystemTime(char * time = nullptr)
{
	auto nNow = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	std::tm* now = std::localtime(&nNow);
	//fprintf(obj->_file, "%s", ctime(&nNow));//输出到文件
	if (time != nullptr)
	{
		sprintf(time, "%d-%02d-%02d %02d:%02d:%02d",	now->tm_year + 1900,//年
											now->tm_mon + 1,	//月
											now->tm_mday,		//日
											now->tm_hour,		//时
											now->tm_min,		//分
											now->tm_sec);		//秒
	}
	return now;
}

int rand(int a, int b)
{
	std::random_device rd;//随机数发生器
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> dist(a, b);
	return dist(gen);
	//return (long)random(a, b);
}

#endif // !GLOBAL_FUNCTIONfunction


#endif