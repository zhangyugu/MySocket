#ifndef CLEE_TIME_STAMP_HPP
#define CLEE_TIME_STAMP_HPP
#include <chrono>
#include <functional>//std::mum_fn
#include <thread>//std::线程
#include <ctime>
//#include <windows.h>
using namespace std::chrono;

//定时器
class CELLTimer
{
public:
	CELLTimer(): _bstart(false){

	}

	CELLTimer(std::function<void()> fun, size_t time): _fun(fun), 
		_time(time), _bstart(false){
		
		 Start();
	}
	~CELLTimer() {
		Stop();
	}

	void Start()
	{
		if (_bstart)
			return;
		_bstart = true;
		std::thread t1(std::mem_fn(&CELLTimer::loop), this);
		t1.detach();

		
	}

	void Stop()
	{
		_bstart = false;
	}

	void setTime(size_t time)
	{
		_time = std::chrono::milliseconds(time);
	}

private:
	void loop()
	{
		
		while (_bstart)
		{
			std::this_thread::sleep_for(_time);
			_fun();
		}
	}

private:
	std::function<void()> _fun;
	bool _bstart;
	std::chrono::milliseconds _time;
};



class CELLTime
{
public:
	//当前时间(毫秒)
	static time_t getNowinMilliSec()
	{
		//return duration_cast<microseconds>(high_resolution_clock::now().time_since_epoch()).count() * 0.001;
		return duration_cast<milliseconds>(high_resolution_clock::now().time_since_epoch()).count();
	}
	static std::tm * getSystemTime(char * time = nullptr)
	{
		auto nNow = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		std::tm* now = std::localtime(&nNow);
		//fprintf(obj->_file, "%s", ctime(&nNow));//输出到文件
		if (time != nullptr)
		{
			sprintf(time, "%d-%02d-%02d %02d:%02d:%02d", now->tm_year + 1900,//年
				now->tm_mon + 1,	//月
				now->tm_mday,		//日
				now->tm_hour,		//时
				now->tm_min,		//分
				now->tm_sec);		//秒
		}
		return now;
	}

};


class CELLTimestamp
{
public:
	CELLTimestamp()
	{
		//QueryPerformanceFrequency(&_frequency);
		//QueryPerformanceCounter(&_startCount);
		update();
	}
	~CELLTimestamp()
	{

	}

	void update()
	{
		//QueryPerformanceCounter(&_startCount);
		_begin = high_resolution_clock::now();
	}

	//获取秒
	double getElapsedSecond()
	{
		return this->getElapsedTimeInMicroSec() * 0.000001;
	}

	//获取毫秒
	double getElapsedTimeInMilliSec()
	{
		return this->getElapsedTimeInMicroSec() * 0.001;
	}

	//获取微妙
	long long getElapsedTimeInMicroSec()
	{
		/*
		LARGE_INTEGER endCount;
		QueryPerformanceCounter(&endCount);

		double startTimeInMicroSec = _startCount.QuadPart * (1000000.0 / _frequency.QuadPart);
		double endTimeInMicroSec   = endCount.Quadpart * (1000000.0 / _frequency.QuadPart);

		return endTimeInMicroSec - startTimeInMicroSec;
		*/
		return duration_cast<microseconds>(high_resolution_clock::now() - _begin).count();
	}

protected:
	//LARGE_INTEGER _frequency;
	//LARGE_INTEGER _startCount;
	time_point<high_resolution_clock> _begin;
};

#endif