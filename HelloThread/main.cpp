#include <iostream>
#include <thread>
#include <mutex>//�߳���
#include <atomic>//ԭ�Ӳ���
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
		//�������Խ���;
		//std::lock_guard<std::mutex> lg(mutex);
		//mutex.lock();
		//Sleep(1);
		++sum;
		
		//std::cout << "Holle, " << n << " thread." << i << std::endl;
		//mutex.unlock();
	}//����: �̰߳�ȫ  �̲߳���ȫ


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

	std::cout << "sum = " << sum << "   ��ʱ(����): " << rtiem.getElapsedTimeInMilliSec() << std::endl;

	rtiem.update();
	sum = 0;
	for (int i = 0; i < 40000000; i++)
	{
		++sum;
	}
	//for (int i = 0; i < 5; i++)
	std::cout << "main sum = " << sum << "   ��ʱ(����): " <<rtiem.getElapsedTimeInMilliSec() << std::endl;

	std::cout << "Hello, main Thread.\n";

	getchar();
	return 0;
}