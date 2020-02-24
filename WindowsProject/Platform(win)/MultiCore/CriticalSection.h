#pragma once
#include"GameCodeStd.h"
#include<Windows.h>

class CriticalSection : public Noncopyable
{
protected:
	mutable CRITICAL_SECTION m_cs;

public:
	CriticalSection()
	{
		InitializeCriticalSection(&m_cs);
	}

	~CriticalSection()
	{
		DeleteCriticalSection(&m_cs);
	}

	void Lock()
	{
		EnterCriticalSection(&m_cs);
	}

	void Unlock()
	{
		LeaveCriticalSection(&m_cs);
	}


};

//Helper class allows automatic locking/ unlocking of a critical resource 
//protected by a critical section
class ScopedCriticalSection : public Noncopyable
{
private:
	CriticalSection& m_csRsource;
public:
	ScopedCriticalSection(CriticalSection& csResource) :m_csRsource(csResource)
	{
		m_csRsource.Lock();
	}

	~ScopedCriticalSection()
	{
		m_csRsource.Unlock();
	}
};

//ConcurrentQueue is based upon the code written by Anthony williams

template<typename Data>
class ConcurrentQueue
{
private:
	std::queue<Data> m_queue;
	CriticalSection m_cs;
	HANDLE m_dataPushedEvent;
public:
	ConcurrentQueue()
	{
		m_dataPushedEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	}

	void Push(const Data& data)
	{
		//Automatic locking and unlocking the mutex
		{
			ScopedCritcicalSection locker(m_cs);
			m_queue.push(data);
		}
		PulseEvent(m_dataPushedEvent);

	}

	bool Empty() const
	{
		ScopedCriticalSection locker(m_cs);
		return m_queue.empty();
	}

	bool Pop(Data& popped)
	{
		ScopedCriticalSection locker(m_cs);
		if (m_queue.empty())
			return false;
		popped = m_queue.front();
		m_queue.pop();
		return true;
	}

	void WaitPop(Data& popped)
	{
		ScopedCriticalSection locker(m_cs);
		while (m_queue.empty)
		{
			WaitForSingleObject(m_dataPushedEvent);
		}

		popped = m_queue.front();
		m_queue.pop();
	}

};