#pragma once
#ifdef _DEBUG
#include"Utilities/String.h"

#define MEMORY_WATCHER_DECLARATION()\
public:\
	static long s_numAllocs;\
	static void* operator new(size_t size, int memBlock = _NORMAL_BLOCK, const char* filename = "<unknown>", int lineNum = 0); \
	static void operator delete(void* ptr); \
	static void* operator new[](size_t size, int memBlock = _NORMAL_BLOCK, const char* filename = "<unknown>", int lineNum = 0;); \
	static void operator delete[](void* ptr);\
private:

	#define MEMORY_WATCHER_DEFINITION(_className_)\
	long _claseName_::s_numAllocs= 0;\
	void* _className_::operator new(size_t size, int memBlock, const char* fileName, int lineNum)\
	{\
		void* pMem= malloc(size);\
		++s_numAllocs;\
		LOG("MemoryWatcher", "NEW: 0x"+ ToStr(reinterpret_cast<unsigned long>(pMem), 16)+ " "+ ToStr(size)+ "(x"+ ToStr(s_numAllocs) + ")->"+ std::string(fileName)+ ":"+ ToStr(lineNum));\
		return pMem;\
	}\
	\
	void _className_::operator delete(void* ptr)\
	{\
		LOG("MemoryWatcher", "DELETE: 0x"+ ToStr(reinterpret_cast<unsigned long>(ptr), 16)+ "(x" + ToStr(s_numAllocs)+ ")");\
		free(ptr);\
		--s_numAllocs;\
	}\
	\
	void* _className_::operator new[](size_t size, int memBlock, const char* fileName, int lineNum)\
	{\
		void* pMem= malloc(size);\
		LOG("MemoryWatcher", "NEW:0x"+ ToStr(reinterpret_cast<unsigned long>(pMem), 16)+ " "+ "(x"+ ToStr(s_numAllocs)+ ")->"+ std::string(filename)+ ":"+ ToStr(lineNum));\
		return pMem;\
	}\
	\
	void _className_::operator delete[](void* ptr)\
	{\
		LOG("MemoryWatcher", "DELETE: 0x"+ ToStr(reinterpret_cast<unsigned long>(ptr), 16)+ " "+ ToStr(size) +"(x"+ ToStr(s_numAllocs));\
		free(ptr);\
		--s_numAllocs;\
	}\

#else
	#define MEMORY_WATCHER_DECLARATION() (sizeof(void*))
	#define MEMORY_WATCHER_DEFINITION(_classname_) (sizeof(_className_))
#endif // _DEBUG


