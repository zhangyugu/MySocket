//�ڴ���� �ڴ��
#ifndef MEMORY_MGR_HPP
#define MEMORY_MGR_HPP
#include "GlobalDef.hpp"



#define MAX_MENORY_SIZE 128
//class MemoryBlock;
class MemoryAlloc;
//template <size_t nSize, size_t nBlockSize> class MemoryAlloctor;

//�ڴ�� ��С��Ԫ
class MemoryBlock
{
public:
	MemoryBlock();
	~MemoryBlock();
public:
	//�ڴ����
	int _nId;
	//���ô���
	int _nRef;
	//�����ڴ��
	MemoryAlloc *_pAlloc;
	//��һ��λ��
	MemoryBlock *_pNext;
	//�Ƿ����ڴ����
	bool _bPool;
private:
	//Ԥ��ռλ
	char cNULL[3];
};

//int a = sizeof MemoryBlock;

//�ڴ��
class MemoryAlloc
{
public:
	MemoryAlloc();
	~MemoryAlloc();
	//�����ڴ�
	void * memalloc(size_t size);
	void memfree(void * pMem);
	//��ʼ��
	void initMemory();
protected:
	//�ڴ�ص�ַ
	char *_pBuf;
	//ʹ���α�  NextΪ��ʱ�����޿����ڴ�
	MemoryBlock *_pVernier;
	//�ڴ浥Ԫ��С
	size_t _nSize;
	//�ڴ浥Ԫ����
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


//�ڴ����  ����ģʽ
class MemoruMgr
{
public:
	static MemoruMgr* instance();
	//�����ڴ�
	void * memalloc(size_t size);
	void memfree(void * memory);
private:
	//��ʼ���ڴ��ӳ������
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
