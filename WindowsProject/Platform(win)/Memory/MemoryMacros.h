#pragma once
#include"MemoryPool.h"

//Some macros to allow classes to use memory pool

//Memory pool declaration, used in the class declaration
#define MEMORYPOOL_DECLARATION(__defaultNumChunks__) \
public:\
	static MemoryPool* s_pMemoryPool;\
	static void InitMemoryPool(unsigned int numChunk= __defaultNumChunks__, const char* debugName= 0 );\
	static void DestroyMemoryPool();\
	static void* operator new(size_t size);\
	static void operator delete(void* ptr);\
	static void* operator new[](size_t size);\
	static void operator delete[](void* ptr);\
private:


//Definition of Memory pool methods declared in MEMORY_DECLARATION()
#define MEMORYPOOL_DEFINITION(_className_) \
MemoryPool* _className_::s_pMemoryPool= NULL;\
void _className_::InitMemoryPool(unsigned int numChunks, const char* debugName= 0 )\
{\
	if(s_pMemoryPool!= NULL)\
	{\
			ERROR("s_pMemoryPool is not NULL"); \
			SAFE_DELETE(s_pMemoryPool);\
	}\
	s_pMemoryPool= New MemoryPool;\
	if(debugName)\
		s_pMemoryPool->SetDebugName(debugName);\
	else\
		s_pMemoryPool->SetDebugName(#_className_);\
	s_pMemoryPool->Init(sizeof(_className_), numChunks);\
}\
void _className_::DestroyMemoryPool()\
{\
	ASSERT(s_pMemoryPool!=NULL);\
	SAFE_DELETE(s_pMemoryPool);\
}\
void* _className_::operator new(size_t size)\
{\
	ASSERT(s_pMemoryPool);\
	void* pMem= s_pMemoryPool->Alloc();\
	return pMem;\
}\
void _className_::operator delete(void* ptr)\
{\
	ASSERT(s_pMemoryPool);\
	s_pMemoryPool->Free(ptr);\
}\
void* _className_::operator new[](size_t size)\
{\
	 ASSERT(s_pMemoryPool); \
     void* pMem = s_pMemoryPool->Alloc(); \
     return pMem; \
}\
void _className_::operator delete[](void* ptr)\
{\
	ASSERT(s_pMemoryPool);\
	s_pMemoryPool->Free(ptr);\
}

//This macro defines a static class that automatically initializes a 
//memory pool at global startup and destroys it at gloabl destruction time
//it can gets around the requirement of manully initializing and destroying
#define MEMORYPOOL_AUTOINIT_DEBUGNAME(_className_, _numChunks_, _debugName_) \
class _className_ ## _AutoInitializedMemoryPool\
{\
public:\
	_className_ ## _AutoInitializedMemoryPool();\
	~_className_ ## _AutoInitializedMemoryPool();\
};\
_className_ ## _AutoInitializedMemoryPool::_className_ ## _AutoInitializedMemoryPool()\
{\
	_className_::InitMemoryPool(_numChunks_, _debugName_);\
}\
_className_ ## _AutoInitializedMemoryPool::~_className_ ## _AutoInitializedMemoryPool()\
{\
	_className_::DestroyMemoryPool();\
}\
static _className_ ## _AutoInitializedMemoryPool s_ ## _className_ ## _AutoInitializedMemoryPool;


#define MEMORYPOOL_AUTOINIT(_className_, _numChunks_) MEMORYPOOL_AUTOINIT_DEBUGNAME(_className_, _numChunks_, #_className_)








