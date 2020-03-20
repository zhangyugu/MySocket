#include <iostream>
#ifdef _WIN32
#include "mysql.h"
#else
#include "mysql/mysql.h"
#endif
#include <vector>



class MyMySql
{
public:
	MyMySql() {
		mysql_init(&_mysql);//初始化数据库
		//初始化数据库  
		if (0 != mysql_library_init(0, NULL, NULL))
			std::cout << "mysql_library_init()failed" << std::endl;
		
	//设置编码方式
		mysql_options(&_mysql, MYSQL_SET_CHARSET_NAME, "gbk");
		//连接数据库
		//判断如果连接失败就输出连接失败。
		//if (mysql_real_connect(&_mysql, "localhost", "root", "110064", "my_test", 3306, NULL, 0) == NULL)
		//{
		//	printf("连接失败!\n");
		//}
	}
	~MyMySql() {
		//释放结果集
		if (_res)
			mysql_free_result(_res);
		//关闭数据库
		mysql_close(&_mysql);
		mysql_library_end();
	}
	//连接
	bool connect(const char * host, const char *user, const char *passwd, const char *db,
		unsigned int port = 3306, const char *unix_socket = nullptr, unsigned long clientflag = 0)
	{
		if (mysql_real_connect(&_mysql, host, user, passwd, db, port, unix_socket, clientflag) == NULL)
		{
			//printf("连接失败!\n");
			return false;
		}
		return true;
	}
	//查询
	std::vector<std::vector<char*>> query(const char *select)
	{
		//查询数据"select * from student"
		mysql_query(&_mysql, select);
		//获取结果集
		_res = mysql_store_result(&_mysql);
		res2vec();
		return _vec_res;
		
	}
	void print()
	{		
		for (auto &x : _vec_res)
		{
			for (auto &n : x)
			{
				if (n)
					printf("%s  ", n);
			}
			printf("\n");
		}
	}
	void res2vec()
	{	
		_vec_res.clear();
		if (_res)
		{	
			MYSQL_ROW _row;  //char** 二维数组，存放一条条记录
			//给ROW赋值，判断ROW是否为空，不为空就打印数据。
			while (_row = mysql_fetch_row(_res))
			{
				std::vector<char*> tmparr;
				for (int i = 0, num = mysql_num_fields(_res); i < num; i++)
				{
					if (_row[i])
						tmparr.push_back(_row[i]);
				}
				_vec_res.push_back(tmparr);
			}
		}
	}
	//上次UPDATE、DELETE或INSERT查询更改／删除／插入的行数
	uint64_t affected()
	{
		return mysql_affected_rows(&_mysql);
	}
	//事务自动提交
	bool autocommit(bool mode)
	{
		return mysql_autocommit(&_mysql, mode);
	}
	//提交事务
	bool commit()
	{
		return mysql_commit(&_mysql);
	}

	const char * error()
	{
		return mysql_error(&_mysql);
	}

	void printf_error()
	{
		printf("ErrorCode: %d, ErrorMessage: %s\n", mysql_errno(&_mysql), mysql_error(&_mysql));	
	}

private:
	MYSQL _mysql;    //一个数据库结构体
	MYSQL_RES* _res; //一个结果集结构体	
	std::vector<std::vector<char*>> _vec_res;
};