#pragma once
#ifndef CELL_THREAD_HPP
#define CELL_THREAD_HPP
#include "CellSemaphore.hpp"
#include <functional>

class CellThread
{
private:
	typedef std::function<void(CellThread*)> EvenCall;

public:
	CellThread(): _isRun(false), _OnStart(nullptr),
	_OnDestory(nullptr), _OnRun(nullptr)
	{

	}
	~CellThread()
	{

	}
	static void sleep(int time)
	{
		std::chrono::milliseconds t(time);
		std::this_thread::sleep_for(t);
	}
	//启动线程
	void Start(EvenCall OnRun     = nullptr,
			   EvenCall OnStart   = nullptr,
			   EvenCall OnDestory = nullptr)
	{
		if (!_isRun)
		{

			std::lock_guard<std::mutex> lg(_mutex);
			_isRun = true;

			if (OnStart)
				_OnStart = OnStart;
			if (OnRun)
				_OnRun = OnRun;
			if (OnDestory)
				_OnDestory = OnDestory;

			std::thread t(std::mem_fn(&CellThread::OnWord), this);
			t.detach();

			
		}
	}
	//关闭线程
	void Close()
	{
		std::lock_guard<std::mutex> lg(_mutex);
		if (_isRun)
		{
			_isRun = false;
			_sem.wait();
		}
	}

	

	//线程是否运行中
	bool isRun()
	{
		return _isRun;
	}
private:
	//线程运行函数
	void OnWord()
	{
		if (_OnStart)
			_OnStart(this);
		if (_OnRun)
			_OnRun(this);
		if (_OnDestory)
			_OnDestory(this);

		_sem.wakeup();
		Exit();
	}

	void Exit()
	{
		std::lock_guard<std::mutex> lg(_mutex);
		if (_isRun)
			_isRun = false;
	}
private:
	EvenCall _OnStart;
	EvenCall _OnDestory;
	EvenCall _OnRun;
	//控制线程终止、退出
	CellSemaphore _sem;
	//线程锁
	std::mutex _mutex;
	//线程是否运行
	bool _isRun;

};




#endif // !CELL_THREAD_HPP
