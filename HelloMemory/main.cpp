#include "Alloc.h"
#include "CELLObjectPool.hpp"
#include "GlobalDef.hpp"

//$(SolutionDir)HelloMemory

std::mutex mutex;
const int tConut = 4;
const int nConut = 100000 / tConut;
//int sum = 0;
void testThread(int n)
{
	char *data[nConut];
	for (size_t i = 0; i < nConut; i++)
	{
		data[i] = new char[(rand() % 128) + 1];
	}
	for (size_t i = 0; i < nConut; i++)
	{
		delete[] data[i];
	}

	return;
	//for (int i = 0; i < nConut; i++)
	//{

		
		//作用域自解锁;
		//std::lock_guard<std::mutex> lg(mutex);
		//mutex.lock();
		//Sleep(1);
		

		//std::cout << "Holle, " << n << " thread." << i << std::endl;
		//mutex.unlock();
	//}//函数: 线程安全  线程不安全


}

class ClassA : public ObjectPoolBase<ClassA, 10>
{
public:
	ClassA(int t)
	{
		i = t;
		std::cout << "ClassA\n";
	}
	~ClassA() 
	{
		std::cout << "~ClassA\n";
	}

private:
	int i;
};

class ClassB : public ObjectPoolBase< ClassB, 10>
{
public:
	ClassB(int s, int b) {
		i = s;
		n = b;
		std::cout << "ClassB\n";
	}
	~ClassB() {
		std::cout << "~ClassB\n";
	}

private:
	int i, n;
};



int main()
{
	/*
	std::thread t[tConut];// (testThread, 0);
	CELLTimestamp rtiem;
	for (int i = 0; i < tConut; i++)
		t[i] = std::thread(testThread, i);

	//t.detach();
	for (int i = 0; i < tConut; i++)
		t[i].join();

	std::cout <<  "耗时(毫秒): " << rtiem.getElapsedTimeInMilliSec() << std::endl;


	return 0;
	rtiem.update();

	for (int i = 0; i < 40000000; i++)
	{
		
	}
	//for (int i = 0; i < 5; i++)
	std::cout <<  "   耗时(毫秒): " << rtiem.getElapsedTimeInMilliSec() << std::endl;

	std::cout << "Hello, main Thread.\n";
	*/

	/*const int count = 12;
	ClassA *arr[count];
	for (int i = 0; i < count; i++)
	{
		arr[i] = new ClassA(1);
	}
	for (int i = 0; i < count; i++)
	{
		delete arr[i];
	}*/
	//ClassA *al = new ClassA(1);
	//delete al;

	//ClassA *a2 = ClassA::createObject(2);
	//ClassA::destroyObject(a2);


	//ClassB *bl = new ClassB(3,4);
	//delete bl;

	//ClassB *b2 = ClassB::createObject(3, 4);
	//ClassB::destroyObject(b2);
	
	/*std::vector< std::shared_ptr< ClassA >> arr;
	for (int i = 0; i < 10 ;i++)
	{
		arr.push_back(std::shared_ptr< ClassA >(new ClassA(1)));
	}

	arr.clear();*/
	ClassA ca(1);
	std::vector<ClassA> arr;
	{
		
		arr.push_back(ca);
		arr.erase(arr.begin());
	}
	

	getchar();
	return 0;
}