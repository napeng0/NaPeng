#include"GameCodeStd.h"
#include"MemoryPool.h"
#include"Utilities\String.h"
#include<stdlib.h>

const static size_t CHUNK_HEADER_SIZE = sizeof(unsigned char*);

MemoryPool::MemoryPool()
{
	Reset();
}

MemoryPool::~MemoryPool()
{
	Destroy();
}

bool MemoryPool::Init(unsigned int chunkSize, unsigned int numChunks)
{
	if (m_ppRawMemoryArray)
		Destroy();

	m_ChunkSize = chunkSize;
	m_NumChunks = numChunks;

	if (GrowMemoryArray())
		return true;
	return false;
}

void MemoryPool::Destroy()
{
#ifdef _DEBUG
	std::string str;
	if (m_NumAllocs != 0)
		str = "(" + ToStr(m_NumAllocs) + ")";
	unsigned long totalNumChunks = m_NumChunks * m_MemArraySize;
	unsigned long wastedMem = (totalNumChunks - m_AllocPeak)* m_ChunkSize;
	str += ("Destorying memory pool:(" + GetDebugName() + ":" + ToStr((unsigned long)m_ChunkSize) + ")=" + ToStr(m_AllocPeak) + "/" + ToStr((unsigned long)totalNumChunks) + "(" + ToStr(wastedMem) + "byte wasted\n");
	
	//By now the custom logger is not initialized so the OS version logger 
	//is used here
	OutputDebugStringA(str.c_str());
#endif // _DEBUG

	//Free all memory
	for (unsigned int i = 0; i < m_MemArraySize; ++i)
	{
		free(m_ppRawMemoryArray[i]);
	}
	free(m_ppRawMemoryArray);

	//Reset member variables
	Reset();

}


void* MemoryPool::Alloc()
{
	//If memory blocks were run out, grow the pool
	if (!m_pHead)
	{
		if (!m_AllowResize)
			return NULL;
		if (!GrowMemoryArray())
			return NULL;
	}

	//Allocation tracker
#ifdef _DEBUG
	++m_NumAllocs;
	if (m_NumAllocs > m_AllocPeak)
		m_AllocPeak = m_NumAllocs;
#endif

	unsigned char* pRet = m_pHead;
	m_pHead = GetNext(m_pHead);
	return (pRet + CHUNK_HEADER_SIZE);

}

void MemoryPool::Free(void* pMem)
{
	if (pMem != NULL)
	{
		//Move back to point to the head
		unsigned char* pBlock = (unsigned char*)pMem - CHUNK_HEADER_SIZE;
		//Push the chunk to the front of the list
		SetNext(pBlock, m_pHead);
		m_pHead = pBlock;
	}

#ifdef _DEBUG
	--m_NumAllocs;
	ASSERT(m_NumAllocs >= 0);
#endif // DEBUG

}


void MemoryPool::Reset()
{
	m_ppRawMemoryArray = NULL;
	m_pHead = NULL;
	m_ChunkSize = 0;
	m_NumChunks = 0;
	m_MemArraySize = 0;
	m_AllowResize = true;
#ifdef _DEBUG
	m_AllocPeak = 0;
	m_NumAllocs = 0;
#endif // DEBUG

}

bool MemoryPool::GrowMemoryArray()
{
#ifdef _DEBUG
	std::string str("Growing memory pool:(" + GetDebugName() + ":" + ToStr((unsigned long)m_ChunkSize) + ")=" + ToStr((unsigned long)m_MemArraySize + 1) + "\n");
	OutputDebugStringA(str.c_str());
#endif // _DEBUG
	
	size_t allocSize = sizeof(unsigned char*)* (m_MemArraySize + 1);
	unsigned char** ppNewMemArray = (unsigned char**)malloc(allocSize);
	if (!ppNewMemArray)
		return false;
	for (unsigned int i = 0; i < m_MemArraySize; ++i)
	{
		ppNewMemArray[i] = m_ppRawMemoryArray[i];
	}
	ppNewMemArray[m_MemArraySize] = AllocateNewMemoryBlock();
	if (m_pHead)
	{
		unsigned char* pCurr = m_pHead;
		unsigned char* pNext = GetNext(m_pHead);
		while (pNext)
		{
			pCurr = pNext;
			pNext = GetNext(pNext);
		}
		SetNext(pCurr, ppNewMemArray[m_MemArraySize]);
	}
	else
	{
		m_pHead = ppNewMemArray[m_MemArraySize];
	}

	if (m_ppRawMemoryArray)
		free(m_ppRawMemoryArray);
	m_ppRawMemoryArray = ppNewMemArray;
	++m_MemArraySize;

	return true;
	

}


unsigned char* MemoryPool::AllocateNewMemoryBlock()
{
	size_t blockSize = m_ChunkSize + CHUNK_HEADER_SIZE;
	size_t trueSize = blockSize * m_NumChunks;

	unsigned char* pNewblock = (unsigned char*)malloc(trueSize);
	if (!pNewblock)
		return NULL;

	//Link the chunks in this block
	unsigned char* pEnd = pNewblock + trueSize;
	unsigned char* pCurr = pNewblock;
	while (pCurr < pEnd)
	{
		unsigned char* pNext = pCurr + blockSize;
		if (pNext < pEnd)
			SetNext(pCurr, pNext);
		pCurr += blockSize;
	}

	return pNewblock;
}


unsigned char* MemoryPool::GetNext(unsigned char* pBlock)
{
	unsigned char**ppChunkHeader = (unsigned char**)pBlock;
	return ppChunkHeader[0];
}


void MemoryPool::SetNext(unsigned char* pBlock, unsigned char* pNext)
{
	unsigned char** ppChunkHeader = (unsigned char**)pBlock;
	ppChunkHeader[0] = pNext;
}


