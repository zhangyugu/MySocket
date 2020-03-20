//内存管理 内存池
#ifndef MEMORY_MGR_HPP
#define MEMORY_MGR_HPP
#include "GlobalDef.hpp"



#define MAX_MENORY_SIZE 128
//class MemoryBlock;
class MemoryAlloc;
//template <size_t nSize, size_t nBlockSize> class MemoryAlloctor;

//内存块 最小单元
class MemoryBlock
{
public:
	MemoryBlock();
	~MemoryBlock();
public:
	//内存块编号
	int _nId;
	//引用次数
	int _nRef;
	//所属内存池
	MemoryAlloc *_pAlloc;
	//下一块位置
	MemoryBlock *_pNext;
	//是否在内存次中
	bool _bPool;
private:
	//预留占位
	char cNULL[3];
};

//int a = sizeof MemoryBlock;

//内存池
class MemoryAlloc
{
public:
	MemoryAlloc();
	~MemoryAlloc();
	//申请内存
	void * memalloc(size_t size);
	void memfree(void * pMem);
	//初始化
	void initMemory();
protected:
	//内存池地址
	char *_pBuf;
	//使用游标  Next为空时即已无空闲内存
	MemoryBlock *_pVernier;
	//内存单元大小
	size_t _nSize;
	//内存单元数量
	size_t _nBlockSize;
	std::mutex _mutex;
};

template <size_t nSize, size_t nBlockSize> 
class MemoryAlloctor: public MemoryAlloc
{
public:
	MemoryAlloctor();
	~MemoryAlloctor();
protected:
private:
};


//内存管理  单例模式
class MemoruMgr
{
public:
	static MemoruMgr* instance();
	//申请内存
	void * memalloc(size_t size);
	void memfree(void * memory);
private:
	//初始化内存池映射数组
	void init_szAlloc(int nBegin, int nEnd, MemoryAlloc *pMenA);
private:
	MemoryAlloctor<64, 1000000> _mem64;
	MemoryAlloctor<128, 1000000> _mem128;
	//MemoryAlloctor<256, 1000000> _mem256;
	//MemoryAlloctor<512, 1000000> _mem512;
	//MemoryAlloctor<1024, 1000000> _mem1024;
	MemoryAlloc * _szAlloc[MAX_MENORY_SIZE + 1];
	
private:
	MemoruMgr();
	~MemoruMgr();
};


#endif // !MEMORY_MGR_HPP
