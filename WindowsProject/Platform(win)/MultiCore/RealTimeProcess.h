#pragma once
#include"MainLoop\Process.h"

class RealtimeProcess : public Process
{
protected:
	HANDLE m_hThread;
	DWORD m_ThreadID;
	int m_ThreadPriority;

public:
	RealtimeProcess(int priority = THREAD_PRIORITY_NORMAL);
	virtual ~RealtimeProcess(void) { CloseHandle(m_hThread); }
	static DWORD WINAPI ThreadProc(LPVOID lpParam);

protected:
	virtual void VOnInit(void);
	virtual void VOnUpdate(unsigned long deltaMs) override { }   
	virtual void VThreadProc(void) = 0;							 
};


const int THREADS_COUNT = 20;
const int WRITES_PER_THREAD = 500;
long g_NumRemainingThreads = THREADS_COUNT;