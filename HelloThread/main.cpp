#include <iostream>
#include <thread>
#include <mutex>//线程锁
#include <atomic>//原子操作
#include <Windows.h>
#include "CELLTimestamp.hpp"

std::mutex mutex;
const int tConut = 4;
std::atomic_int sum = 0;
//int sum = 0;
void testThread(int n)
{
	
	for (int i = 0; i < 10000000; i++)
	{
		//作用域自解锁;
		//std::lock_guard<std::mutex> lg(mutex);
		//mutex.lock();
		//Sleep(1);
		++sum;
		
		//std::cout << "Holle, " << n << " thread." << i << std::endl;
		//mutex.unlock();
	}//函数: 线程安全  线程不安全


}

int main()
{
	std::thread t[tConut];// (testThread, 0);
	CELLTimestamp rtiem;
	for (int i = 0; i < tConut; i++)
		t[i] = std::thread(testThread, i);

	//t.detach();
	for (int i = 0; i < tConut; i++)
		t[i].join();

	std::cout << "sum = " << sum << "   耗时(毫秒): " << rtiem.getElapsedTimeInMilliSec() << std::endl;

	rtiem.update();
	sum = 0;
	for (int i = 0; i < 40000000; i++)
	{
		++sum;
	}
	//for (int i = 0; i < 5; i++)
	std::cout << "main sum = " << sum << "   耗时(毫秒): " <<rtiem.getElapsedTimeInMilliSec() << std::endl;

	std::cout << "Hello, main Thread.\n";

	getchar();
	return 0;
}