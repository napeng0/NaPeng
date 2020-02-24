#pragma once
#include<xstring>

//This class represents a single memory pool which split into chunks of equal size
// with a 4 byte header. The header is pointer to next chunk, making the pool a singly-linked
// list of memory chunks
class MemoryPool
{
	unsigned char** m_ppRawMemoryArray;//Array of memory blocks spliting up into chunks
	unsigned char* m_pHead;
	unsigned int m_ChunkSize, m_NumChunks;
	unsigned int m_MemArraySize;//The number of elements in memory pool
	bool m_AllowResize;//Allow resizing the memory pool when it fills up

	//Tracker for debug
#ifdef _DEBUG
	std::string m_DebugName;
	unsigned long m_AllocPeak, m_NumAllocs;
#endif

public:
	MemoryPool();
	~MemoryPool();

	//Allocate the appropriate amout of memory and set up the data strcture
	//the chunk size is the size of each chunk, minus the header
	//total usage will be N* (S+ 4)+ O,where N is the number of chunks,S is 
	//the size of each chunk, O is the overhead of the class
	bool Init(unsigned int chunkSize, unsigned int numChunks);
	void Destroy();

	//Alloc() will retrive a chunk from memory pool, it removes the head of
	//the linked list, set the new head to the next chunk, and returns a poniter
	//to the data section of the old head. If there aren't anymore chunks left
	//it will allocate another block of N chunks, where N is the number of chunk passed
	//into Init()
	void* Alloc();

	//Free() releases a chunk of memory back into memory pool for reuse.
	//The chunk will be inserted to the front of the list,
	void Free(void* pMem);
	unsigned int GetChunkSize() const { return m_ChunkSize; }

	void SetAllowResize(bool allowResize) { m_AllowResize = allowResize; }

	//Debug function
#ifdef _DEBUG
	void SetDebugName(const char* debugName) { m_DebugName = debugName; }
	std::string GetDebugName() const { return m_DebugName; }
#else
	void SetDebugName(const char* debugName) {}
	std::string GetDebugName() const { return std::string("<No Name>"); }
#endif // _DEBUG

private:
	void Reset();

	//Memory allocation helper
	bool GrowMemoryArray();
	unsigned char* AllocateNewMemoryBlock();

	//Linked list management
	unsigned char* GetNext(unsigned char* pBlock);
	void SetNext(unsigned char* pBlock, unsigned char* pNewNext);

	//Don't copy
	MemoryPool(const MemoryPool& memPool) {}

};