#include "MemoryMgr.h"

//内存块 最小单元

MemoryBlock::MemoryBlock() :_nId(0), _nRef(0), _pAlloc(nullptr),
_pNext(nullptr), _bPool(false)
{

}
MemoryBlock::~MemoryBlock()
{

}
//int a = sizeof MemoryBlock;

//内存池

MemoryAlloc::MemoryAlloc() : _pBuf(nullptr), _pVernier(nullptr), _nSize(0),
_nBlockSize(0)
{
	//initMemory();
}
MemoryAlloc::~MemoryAlloc()
{
	if (_pBuf)
		free(_pBuf);
}
//申请内存
void * MemoryAlloc::memalloc(size_t size)
{
	if (!_pBuf)
	{
		initMemory();
	}
	std::lock_guard<std::mutex> lg(_mutex);
	MemoryBlock *pReturn = nullptr;
	if (nullptr == _pVernier)//如果已经没有了  指向最后一个
	{
		pReturn = (MemoryBlock*)malloc(_nSize + sizeof(MemoryBlock));
		assert(pReturn != nullptr);

		pReturn->_bPool = false;
		pReturn->_nId = -1;
		pReturn->_nRef = 1;
		pReturn->_pAlloc = nullptr;
		pReturn->_pNext = nullptr;
	}
	else
	{
		pReturn = _pVernier;
		_pVernier = _pVernier->_pNext;
		//断言
		assert(0 == pReturn->_nRef);

		pReturn->_nRef = 1;
	}
	xPrintf("Alloc::memalloc: %p, id = %d, size = %d\n", pReturn, pReturn->_nId, (int)size);
	return ((char*)pReturn + sizeof(MemoryBlock));
}

void MemoryAlloc::memfree(void * pMem)
{
	MemoryBlock *pBlock = (MemoryBlock *)((char*)pMem - sizeof(MemoryBlock));
	//断言
	assert(1 == pBlock->_nRef);
	if (--pBlock->_nRef != 0)
	{
		return;
	}
	if (pBlock->_bPool)//是否在池内
	{
		std::lock_guard<std::mutex> lg(_mutex);
		pBlock->_pNext = _pVernier;//被释放的Block指向浮标
		_pVernier = pBlock;//成为新浮标
	}
	else
	{
		free(pBlock);
	}

}

//初始化
void MemoryAlloc::initMemory()
{
	//断言
	assert(nullptr == _pBuf);
	if (_pBuf)
		return;
	//计算内存池大小
	size_t bufsize = (_nSize + sizeof(MemoryBlock)) * _nBlockSize;
	//向系统申请内存池内存
	_pBuf = (char*)malloc(bufsize);
	//断言
	assert(nullptr != _pBuf);

	//初始化内存池  头
	_pVernier = (MemoryBlock *)_pBuf;
	_pVernier->_bPool = true;
	_pVernier->_nId = 0;
	_pVernier->_nRef = 0;
	_pVernier->_pAlloc = this;

	MemoryBlock *temp = _pVernier, *temp2;
	for (size_t i = 1; i < _nBlockSize; i++)
	{
		temp2 = (MemoryBlock *)(_pBuf + ((_nSize + sizeof(MemoryBlock)) * i));
		temp2->_nId = (int)i;
		temp2->_nRef = 0;
		temp2->_pAlloc = this;
		temp2->_pNext = nullptr;
		temp2->_bPool = true;
		temp->_pNext = temp2;//指向下一个
		temp = temp2;//更新
	}

}


template <size_t nSize, size_t nBlockSize>
MemoryAlloctor<nSize, nBlockSize>::MemoryAlloctor()
{
	//内存对齐
	const size_t n = sizeof(void*);
	//(nSize/n) * n  取n的倍数
	_nSize = (nSize / n) * n + (nSize % n ? n : 0);
	_nBlockSize = nBlockSize;
	MemoryAlloc::initMemory();
}

template<size_t nSize, size_t nBlockSize>
MemoryAlloctor<nSize, nBlockSize>::~MemoryAlloctor()
{
}



//内存管理  单例模式

MemoruMgr* MemoruMgr::instance()
{
	static MemoruMgr object;
	return &object;
}
//申请内存
void * MemoruMgr::memalloc(size_t size)
{
	if (size <= MAX_MENORY_SIZE)
	{
		return _szAlloc[size]->memalloc(size);
	}
	else
	{
		MemoryBlock *pReturn = nullptr;
		pReturn = (MemoryBlock*)malloc(size + sizeof(MemoryBlock));
		pReturn->_bPool = false;
		pReturn->_nId = -1;
		pReturn->_nRef = 1;
		pReturn->_pAlloc = nullptr;
		pReturn->_pNext = nullptr;
		xPrintf("Mgr::memalloc: %p, id = %d, size = %d\n", pReturn, pReturn->_nId, (int)size);
		return (char*)pReturn + sizeof(MemoryBlock);
	}


}

void MemoruMgr::memfree(void * memory)
{
	MemoryBlock *pBlock = (MemoryBlock *)((char*)memory - sizeof(MemoryBlock));
	//断言
	/*assert(1 == pBlock->_nRef);

	{
		return;
	}*/
	xPrintf("Mgr::memfree: %p, id = %d\n", pBlock, pBlock->_nId);
	if (pBlock->_bPool)//是否在池内
	{
		pBlock->_pAlloc->memfree(memory);
	}
	else
	{
		if (--pBlock->_nRef == 0)
			free(pBlock);
	}
}

//初始化内存池映射数组
void MemoruMgr::init_szAlloc(int nBegin, int nEnd, MemoryAlloc *pMenA)
{
	for (int n = nBegin; n <= nEnd; n++)
	{
		_szAlloc[n] = pMenA;
	}
}

MemoruMgr::MemoruMgr()
{
	init_szAlloc(0, 64, &_mem64);
	init_szAlloc(65, 128, &_mem128);
	//init_szAlloc(129, 256, &_mem256);
	//init_szAlloc(257, 512, &_mem512);
	//init_szAlloc(513, 1024, &_mem1024);
	//xPrintf("MemoruMgr()");
}
MemoruMgr::~MemoruMgr()
{

}






