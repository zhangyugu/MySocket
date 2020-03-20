//自定义运算符  使用内存池
#ifndef ALLOC_H_
#define ALLOC_H_

void* operator new(size_t size);

void operator delete(void* memory);

void* operator new[](size_t size);

void operator delete[](void* memory);

void* mem_alloc(size_t size);

void  mem_free(void* memory);
#endif
