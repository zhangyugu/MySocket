//�����
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

	template<typename ...Args>//�ɱ��
	static Type* createObject(Args ... args)
	{
		Type *obj = new Type(args...);
		//��������
		return obj;
	}
	static void destroyObject(Type *obj)
	{
		delete obj;
	}

private:
	//typedef CELLObjectPool<Type, nPoolSize> ClassTypePool;//ģ��ʵ��������
	//static ClassTypePool& objectPool(); ����Ҳ��
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
		//��һ��λ��
		NodeHeader *_pNext;
		//�ڴ����
		int _nId;
		//���ô���
		char _nRef;
		//�Ƿ����ڴ����
		bool _bPool;
	private:
		//Ԥ��ռλ
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
	//�������
	void * memalloc(size_t size)
	{
		std::lock_guard<std::mutex> lg(_mutex);
		NodeHeader *pReturn = nullptr;
		if (nullptr == _pVernier)//����Ѿ�û����  ָ�����һ��
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
			//����
			assert(0 == pReturn->_nRef);

			pReturn->_nRef = 1;
		}
		xPrintf("ObjectAlloc::memalloc: %p, id = %d, size = %d\n", pReturn, pReturn->_nId, (int)size);
		return ((char*)pReturn + sizeof(NodeHeader));
	}

	//�ͷŶ���
	void memfree(void * pMem)
	{
		NodeHeader *pBlock = (NodeHeader *)((char*)pMem - sizeof(NodeHeader));
		//����
		assert(1 == pBlock->_nRef);
		if (--pBlock->_nRef != 0)
		{
			return;
		}
		xPrintf("Object::memfree: %p, id = %d\n", pBlock, pBlock->_nId);
		if (pBlock->_bPool)//�Ƿ��ڳ���
		{
			std::lock_guard<std::mutex> lg(_mutex);
			pBlock->_pNext = _pVernier;//���ͷŵ�Blockָ�򸡱�
			_pVernier = pBlock;//��Ϊ�¸���
		}
		else
		{
			delete pBlock;
		}
	}
	//��ʼ�������
	void initPool()
	{
		//����
		assert(nullptr == _pBuf);
		if (_pBuf)
			return;
		//�����ڴ�ش�С
		size_t n = nPoolSize * (sizeof(Type) + sizeof(NodeHeader));
		//�����ڴ�
		_pBuf = new char[n];
		//��ʼ��
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
			temp->_pNext = temp2;//ָ����һ��
			temp = temp2;//����
		}
	}
private:

	//ʹ���α�  NextΪ��ʱ�����޿����ڴ�
	NodeHeader *_pVernier;
	//������ڴ滺������ַ
	char * _pBuf;
	//��
	std::mutex _mutex;
};




#endif // !CELL_OBJECT_POOL_H


