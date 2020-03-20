#include "MemoryMgr.h"

//�ڴ�� ��С��Ԫ

MemoryBlock::MemoryBlock() :_nId(0), _nRef(0), _pAlloc(nullptr),
_pNext(nullptr), _bPool(false)
{

}
MemoryBlock::~MemoryBlock()
{

}
//int a = sizeof MemoryBlock;

//�ڴ��

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
//�����ڴ�
void * MemoryAlloc::memalloc(size_t size)
{
	if (!_pBuf)
	{
		initMemory();
	}
	std::lock_guard<std::mutex> lg(_mutex);
	MemoryBlock *pReturn = nullptr;
	if (nullptr == _pVernier)//����Ѿ�û����  ָ�����һ��
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
		//����
		assert(0 == pReturn->_nRef);

		pReturn->_nRef = 1;
	}
	xPrintf("Alloc::memalloc: %p, id = %d, size = %d\n", pReturn, pReturn->_nId, (int)size);
	return ((char*)pReturn + sizeof(MemoryBlock));
}

void MemoryAlloc::memfree(void * pMem)
{
	MemoryBlock *pBlock = (MemoryBlock *)((char*)pMem - sizeof(MemoryBlock));
	//����
	assert(1 == pBlock->_nRef);
	if (--pBlock->_nRef != 0)
	{
		return;
	}
	if (pBlock->_bPool)//�Ƿ��ڳ���
	{
		std::lock_guard<std::mutex> lg(_mutex);
		pBlock->_pNext = _pVernier;//���ͷŵ�Blockָ�򸡱�
		_pVernier = pBlock;//��Ϊ�¸���
	}
	else
	{
		free(pBlock);
	}

}

//��ʼ��
void MemoryAlloc::initMemory()
{
	//����
	assert(nullptr == _pBuf);
	if (_pBuf)
		return;
	//�����ڴ�ش�С
	size_t bufsize = (_nSize + sizeof(MemoryBlock)) * _nBlockSize;
	//��ϵͳ�����ڴ���ڴ�
	_pBuf = (char*)malloc(bufsize);
	//����
	assert(nullptr != _pBuf);

	//��ʼ���ڴ��  ͷ
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
		temp->_pNext = temp2;//ָ����һ��
		temp = temp2;//����
	}

}


template <size_t nSize, size_t nBlockSize>
MemoryAlloctor<nSize, nBlockSize>::MemoryAlloctor()
{
	//�ڴ����
	const size_t n = sizeof(void*);
	//(nSize/n) * n  ȡn�ı���
	_nSize = (nSize / n) * n + (nSize % n ? n : 0);
	_nBlockSize = nBlockSize;
	MemoryAlloc::initMemory();
}

template<size_t nSize, size_t nBlockSize>
MemoryAlloctor<nSize, nBlockSize>::~MemoryAlloctor()
{
}



//�ڴ����  ����ģʽ

MemoruMgr* MemoruMgr::instance()
{
	static MemoruMgr object;
	return &object;
}
//�����ڴ�
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
	//����
	/*assert(1 == pBlock->_nRef);

	{
		return;
	}*/
	xPrintf("Mgr::memfree: %p, id = %d\n", pBlock, pBlock->_nId);
	if (pBlock->_bPool)//�Ƿ��ڳ���
	{
		pBlock->_pAlloc->memfree(memory);
	}
	else
	{
		if (--pBlock->_nRef == 0)
			free(pBlock);
	}
}

//��ʼ���ڴ��ӳ������
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






