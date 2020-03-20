#include "Alloc.h"
#include "MemoryMgr.h"

void * operator new(size_t size)
{
	return MemoruMgr::instance()->memalloc(size);
}

void operator delete(void * memory)
{
	MemoruMgr::instance()->memfree(memory);
}

void * operator new[](size_t size)
{
	return MemoruMgr::instance()->memalloc(size);
}

void operator delete[](void * memory)
{
	MemoruMgr::instance()->memfree(memory);
}

void * mem_alloc(size_t size)
{
	return malloc(size);
}

void mem_free(void * memory)
{
	free(memory);
}
