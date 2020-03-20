#pragma once
#ifndef CELL_SEMAPHORE_HPP
#define CELL_SEMAPHORE_HPP
#include <condition_variable>//条件变量

//信号量
class CellSemaphore
{
public:
	CellSemaphore() : _wait(0), _wakeup(0)
	{


	}
	~CellSemaphore()
	{

	}
	//阻塞当前线程
	void wait()
	{

		std::unique_lock<std::mutex> lock(_mutex);
		if (--_wait < 0)
		{
			_cv.wait(lock, [this]()->bool {
				return _wakeup > 0;
			});
			--_wakeup;
		}

	}
	//唤醒当前线程
	void wakeup()
	{
		if (++_wait <= 0)
		{
			++_wakeup;
			_cv.notify_one();
		}
	}

private:
	std::mutex _mutex;
	std::condition_variable _cv;
	int _wait, _wakeup;


};

#endif // !CELL_SEMAPHORE_HPP
