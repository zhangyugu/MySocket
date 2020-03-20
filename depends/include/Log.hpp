#ifndef _LOG_HPP_
#define _LOG_HPP_
#include "GlobalDef.hpp"
#include "CellTaskServer.hpp"

#ifdef _DEBUG
#ifndef xPrintf
#define xPrintf(...) printf(__VA_ARGS__)
#endif
#else
#ifndef xPrintf
#define xPrintf(...)
#endif
#endif//_DEBUG

class Log
{
public:
	static Log* Instance()
	{
		static Log obj;
		return &obj;
	}

	static void Info(const char* pStr)
	{
		Log* obj = Instance();
		obj->_task.addTask([obj, pStr]() {
			if (obj->_file)
			{
				obj->outtime();
				fprintf(obj->_file, "%s", pStr);//输出到文件
				fflush(obj->_file);//刷新流
			}
			xPrintf("%s", pStr);
		});
		
	}

	template<typename ...Args>
	static void Info(const char* pformat, Args ... args)
	{

		Log* obj = Instance();
		obj->_task.addTask([obj, pformat, args...]() {
			if (obj->_file)
			{
				obj->outtime();
				fprintf(obj->_file, pformat, args...);
				fflush(obj->_file);//刷新流
			}
			printf(pformat, args...);
		});
		
	}

	inline void outtime()
	{
		auto nNow = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		std::tm* now = std::localtime(&nNow);
		fprintf(_file, "[%d/%d/%d-%d:%d:%d]", now->tm_year + 1900, now->tm_mon + 1,now->tm_mday, 
			now->tm_hour + 1, now->tm_min, now->tm_sec);//输出到文件
	}
	//Debug
	//Warring
	//Error
	void setLogPath(const char* path, const char* mode)
	{
		if (_file)
		{
			//关闭
			_file = nullptr;
		}

		_file = fopen(path, mode);
		if (_file)
			Info("Log::setLogPath success, <%s, %s> \n", path, mode);
		else
			Info("Log::setLogPath failed, <%s, %s> \n", path, mode);
	}

private:
	FILE * _file;
	CellTaskServer _task;
private:
	Log(): _file(nullptr){
		_task.Start();
	}
	~Log() {
		if (_file)
			fclose(_file);
		_task.Close();
	}
};




#endif // !_LOG_HPP_
