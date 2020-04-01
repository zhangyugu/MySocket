#ifndef EPOLL_HPP
#define EPOLL_HPP

#include <iostream>
#include <functional>//泛函数 std::mum_fn
#if __linux__
#define EPOLL_ERROR (-1)
#include <sys/epoll.h>
#include <fcntl.h>
class Epoll
{
private:
	typedef std::function<void(int)> Handle;
	epoll_event _event, *_events;
	int _epoll_fd;
	bool _isNormal;//是否正常
	const int _MaxEvents;
public:
	Epoll(int maxevents = 1024) : _events(nullptr), _epoll_fd(-1), _isNormal(false), _MaxEvents(maxevents)
	{
		//printf("最大监听数: %d\n", _MaxEvents);
		create();
	}
	~Epoll()
	{
		if (_events)
			free(_events);
		close(_epoll_fd);
	}
	
	bool isNormal()
	{
		return _isNormal;
	}
	// 注册新描述符
	int add(int fd, void* data, int events = EPOLLIN | EPOLLET) {
		if (_isNormal)
		{
			make_socket_non_blocking(fd);
			_event.data.ptr = data;
			_event.events = events;
			return epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, fd, &_event);			
		}
		else
		{
			printf("epoll未正常启动socket<%d>\n", fd);
			return -1;
		}
			
	}

	// 修改描述符状态
	int tk_epoll_mod(int fd, void* data, int events) {
		if (_isNormal)
		{		
			_event.data.ptr = data;
			_event.events = events;
			return epoll_ctl(_epoll_fd, EPOLL_CTL_MOD, fd, &_event);
		}
		else return -1;
	}

	// 从epoll中删除描述符
	int del(int fd, void* data,int events = EPOLLIN | EPOLLET) {
		if (_isNormal)
		{
			_event.data.ptr = data;
			_event.events = events;
			return epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, fd, &_event);
			
		}
		else return -1;
	}

	// 返回活跃事件数
	int wait(int timeout)
	{
		if (_isNormal)
		{
			return epoll_wait(_epoll_fd, _events, _MaxEvents, timeout);
			/*if (num < 0)
				return -1;*/
			//for (int i = 0; i < num; ++i)
			//{
			//	if (_events[i].events & EPOLLIN)
			//	{					
			//		if (read)
			//			read(_events[i].data.fd);

			//		//修改sockfd_r上要处理的事件为EPOLLOUT
			//		_event.data.fd = _events[i].data.fd;
			//		_event.events = EPOLLOUT | EPOLLET;
			//		epoll_ctl(_epoll_fd, EPOLL_CTL_MOD, _events[i].data.fd, &_event);
			//	}
			//	else if (_events[i].events & EPOLLOUT)
			//	{
			//		if (write)
			//			write(_events[i].data.fd);
			//		//修改sockfd_w上要处理的事件为EPOLLIN
			//		_event.data.fd = _events[i].data.fd;
			//		_event.events = EPOLLIN | EPOLLET;
			//		epoll_ctl(_epoll_fd, EPOLL_CTL_MOD, _events[i].data.fd, &_event);
			//	}
			//}
			//return num;
		}
		else return -1;
		

	}
	epoll_event* events()
	{
		return _events;
	}
	//// 分发处理函数
	//void tk_handle_events(int epoll_fd, int listen_fd, 
	//	int events_num, char* path) {
	//	for (int i = 0; i < events_num; i++) {
	//		// 获取有事件产生的描述符
	//		//tk_http_request_t* request = (tk_http_request_t*)(events[i].data.ptr);
	//		//int fd = request->fd;
	//		// 有事件发生的描述符为监听描述符
	//		if (fd == listen_fd) {
	//			//accept_connection(listen_fd, epoll_fd, path);
	//		}
	//		else {
	//			// 有事件发生的描述符为连接描述符
	//			// 排除错误事件
	//			if ((_events[i].events & EPOLLERR) || (_events[i].events & EPOLLHUP)
	//				|| (!(_events[i].events & EPOLLIN))) {
	//				close(fd);
	//				continue;
	//			}
	//			// 将请求任务加入到线程池中
	//			//int rc = threadpool_add(tp, do_request, _events[i].data.ptr);
	//			// do_request(events[i].data.ptr);
	//		}
	//	}
	//}

private:
	void create() {
		_epoll_fd = epoll_create1(0);
		if (_epoll_fd > 0)
			_isNormal = true;
		else
		{
			printf("create, epoll_create1_error<%d>\n", _epoll_fd);
			return;
		}
		_events = (epoll_event*)malloc(sizeof(epoll_event) * _MaxEvents);
		if (!_events)
		{
			printf("create, malloc_error<%d>\n", _epoll_fd);
			_isNormal = false;
		}			
	}

	int make_socket_non_blocking(int fd) {
		int flag = fcntl(fd, F_GETFL, 0);
		if (flag == -1)
			return -1;

		flag |= O_NONBLOCK;
		if (fcntl(fd, F_SETFL, flag) == -1)
			return -1;
		return 0;
	}
};




#endif //__linux__
#endif // !CELL_EPOLL_HPP
