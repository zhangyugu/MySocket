#ifndef CELL_TASK_SERVER_HPP
#define CELL_TASK_SERVER_HPP
#include"GlobalDef.hpp"
#include"CellThread.hpp"
#include <list>
#include <queue>

class CellTaskServer
{
private:
	typedef std::function<void()> CellTask;
private:
	//任务链表
	std::queue<CellTask> _taskList;
	//任务缓冲区t
	std::queue<CellTask> _taskBuf;
	//任务缓冲区锁
	std::mutex _mutex;
	CellThread _therad;
public:

	CellTaskServer(bool start = false) 
	{
		if (start)
			Start();

	}
	virtual ~CellTaskServer()
	{	
		Close();
	}
	//添加任务
	void addTask(CellTask pTask)
	{
		std::lock_guard<std::mutex> lg(_mutex);
		_taskBuf.push(pTask);
	}
	//启动服务
	void Start()
	{
		_therad.Start([this](CellThread* pthread) {
			OnRun(pthread);
		});
		//_therad = std::thread(std::mem_fn(&CellTaskServer::OnRun), this);
		//_therad.detach();
	}

	void Close()
	{
		_therad.Close();	
	}

protected:
	//处理函数
	void OnRun(CellThread* pthread)
	{
		while (pthread->isRun())
		{
			//从缓冲区提取任务
			if (!_taskBuf.empty())
			{
				std::lock_guard<std::mutex> lg(_mutex);
				while (!_taskBuf.empty())
				{
					_taskList.push(_taskBuf.front());
					_taskBuf.pop();
				}
			}

			//如果没有任务
			if (_taskList.empty())
			{
				std::chrono::milliseconds t(1);
				std::this_thread::sleep_for(t);
				continue;
			}

			//处理任务
			while (!_taskList.empty())
			{
				_taskList.front()();
				_taskList.pop();
			}
		}
		while (!_taskBuf.empty())
		{
			_taskBuf.pop();
		}
		while (!_taskList.empty())
		{
			_taskList.pop();
		}
	}


};


#endif // !CELL_SEND_TASK_HPP
