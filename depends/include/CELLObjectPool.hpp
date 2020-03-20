//对象池
#pragma once
#ifndef CELL_OBJECT_POOL_H
#define CELL_OBJECT_POOL_H
#include "GlobalDef.hpp"



template <class Type, size_t nPoolSize> class CELLObjectPool;

template <class Type, size_t nPoolSize>
class ObjectPoolBase
{
public:
	ObjectPoolBase()
	{

	}
	~ObjectPoolBase()
	{

	}
	void* operator new(size_t nSize)
	{
		return objectPool()->memalloc(nSize);
	}
	void operator delete(void *p)
	{
		objectPool()->memfree(p);
	}

	template<typename ...Args>//可变参
	static Type* createObject(Args ... args)
	{
		Type *obj = new Type(args...);
		//处理其他
		return obj;
	}
	static void destroyObject(Type *obj)
	{
		delete obj;
	}

private:
	//typedef CELLObjectPool<Type, nPoolSize> ClassTypePool;//模板实例化声明
	//static ClassTypePool& objectPool(); 这样也行
	static CELLObjectPool<Type, nPoolSize>* objectPool()
	{
		static CELLObjectPool<Type, nPoolSize> Pool;
		return &Pool;
	}
	
};


template <class Type, size_t nPoolSize>
class CELLObjectPool
{
	class NodeHeader
	{
	public:
		NodeHeader() : _pNext(nullptr), _nId(0), _nRef(0), _bPool(false) {}
		//下一块位置
		NodeHeader *_pNext;
		//内存块编号
		int _nId;
		//引用次数
		char _nRef;
		//是否在内存次中
		bool _bPool;
	private:
		//预留占位
		char cNULL[2];
	};
public:
	CELLObjectPool() :  _pVernier(nullptr), _pBuf(nullptr)
	{
		initPool();
	}
	~CELLObjectPool()
	{
		if (_pBuf)
			delete[] _pBuf;
	}
	//申请对象
	void * memalloc(size_t size)
	{
		std::lock_guard<std::mutex> lg(_mutex);
		NodeHeader *pReturn = nullptr;
		if (nullptr == _pVernier)//如果已经没有了  指向最后一个
		{
			pReturn = (NodeHeader*)new char[sizeof(Type) + sizeof(NodeHeader)];
			assert(pReturn != nullptr);

			pReturn->_bPool = false;
			pReturn->_nId = -1;
			pReturn->_nRef = 1;
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
		xPrintf("ObjectAlloc::memalloc: %p, id = %d, size = %d\n", pReturn, pReturn->_nId, (int)size);
		return ((char*)pReturn + sizeof(NodeHeader));
	}

	//释放对象
	void memfree(void * pMem)
	{
		NodeHeader *pBlock = (NodeHeader *)((char*)pMem - sizeof(NodeHeader));
		//断言
		assert(1 == pBlock->_nRef);
		if (--pBlock->_nRef != 0)
		{
			return;
		}
		xPrintf("Object::memfree: %p, id = %d\n", pBlock, pBlock->_nId);
		if (pBlock->_bPool)//是否在池内
		{
			std::lock_guard<std::mutex> lg(_mutex);
			pBlock->_pNext = _pVernier;//被释放的Block指向浮标
			_pVernier = pBlock;//成为新浮标
		}
		else
		{
			delete pBlock;
		}
	}
	//初始化对象池
	void initPool()
	{
		//断言
		assert(nullptr == _pBuf);
		if (_pBuf)
			return;
		//计算内存池大小
		size_t n = nPoolSize * (sizeof(Type) + sizeof(NodeHeader));
		//申请内存
		_pBuf = new char[n];
		//初始化
		_pVernier = (NodeHeader *)_pBuf;
		_pVernier->_pNext = nullptr;
		_pVernier->_nId = 0;
		_pVernier->_nRef = 0;
		_pVernier->_bPool = true;

		NodeHeader *temp = _pVernier, *temp2;
		for (size_t i = 1; i < nPoolSize; i++)
		{
			temp2 = (NodeHeader *)(_pBuf + ((sizeof(Type) + sizeof(NodeHeader)) * i));
			temp2->_nId = (int)i;
			temp2->_nRef = 0;
			temp2->_pNext = nullptr;
			temp2->_bPool = true;
			temp->_pNext = temp2;//指向下一个
			temp = temp2;//更新
		}
	}
private:

	//使用游标  Next为空时即已无空闲内存
	NodeHeader *_pVernier;
	//对象池内存缓存区地址
	char * _pBuf;
	//锁
	std::mutex _mutex;
};




#endif // !CELL_OBJECT_POOL_H


