#include"GameCodeStd.h"
#include"RealTimeProcess.h"
#include"EventManager\EventManager.h"
#include"EventManager\Events.h"
#include"MainLoop\ProcessManager.h"
#include<stdio.h>
#include"ResourceCache\ZipFile.h"


DWORD g_MaxLoops = 100000;
DWORD g_ProtectedTotal = 0;
CRITICAL_SECTION g_CriticalSection;


DWORD WINAPI ThreadProc(LPVOID lpParam)
{
	DWORD maxLoops = *(DWORD*)(lpParam);
	DWORD dwCount = 0;
	while (dwCount < maxLoops)
	{
		++dwCount;

		EnterCriticalSection(&g_CriticalSection);
		++g_ProtectedTotal;
		LeaveCriticalSection(&g_CriticalSection);
	}
	return TRUE;
}


void CreateThreads()
{
	InitializeCriticalSection(&g_CriticalSection);

	for (int i = 0; i < 20; i++)
	{
		HANDLE m_hThread = CreateThread(
			NULL,       // Default security attributes
			0,          // Default stack size
			(LPTHREAD_START_ROUTINE)ThreadProc,
			&g_MaxLoops,  // Thread parameter is how many loops
			0, // Default creation flags
			NULL);      // Receive thread identifier
	}
}



RealtimeProcess::RealtimeProcess(int priority)
{
	m_ThreadID = 0;
	m_ThreadPriority = priority;
}



DWORD WINAPI RealtimeProcess::ThreadProc(LPVOID lpParam)
{
	RealtimeProcess* proc = (RealtimeProcess*)(lpParam);
	proc->VThreadProc();
	return TRUE;
}


void RealtimeProcess::VOnInit(void)
{
	Process::VOnInit();
	m_hThread = CreateThread(
		NULL,         // Default security attributes
		0,            // Default stack size
		ThreadProc,   // Thread process
		this,         // Thread parameter is a pointer to the process
		0,            // Default creation flags
		&m_ThreadID); // Receive thread identifier

	if (m_hThread == NULL)
	{
		ERROR("Could not create thread!");
		Fail();
		return;
	}

	SetThreadPriority(m_hThread, m_ThreadPriority);
}


class ProtectedProcess : public RealtimeProcess
{
public:

	static DWORD s_ProtectedTotal;
	static CriticalSection s_CriticalSection;
	DWORD m_MaxLoops;
	ProtectedProcess(DWORD maxLoops) : RealtimeProcess() { m_MaxLoops = maxLoops; }

	virtual void VThreadProc(void);
};


DWORD ProtectedProcess::s_ProtectedTotal = 0;
CriticalSection ProtectedProcess::s_CriticalSection;

void ProtectedProcess::VThreadProc()
{
	DWORD dwCount = 0;

	while (dwCount < m_MaxLoops)
	{
		++dwCount;

		{
			ScopedCriticalSection locker(s_CriticalSection);
			++g_ProtectedTotal;
		}
	}

	Succeed();
}


class UnprotectedProcess : public RealtimeProcess
{
public:
	static DWORD s_UnprotectedTotal;
	DWORD m_MaxLoops;
	UnprotectedProcess(DWORD maxLoops) : RealtimeProcess() { m_MaxLoops = maxLoops; }
	virtual void VThreadProc();
};
DWORD UnprotectedProcess::s_UnprotectedTotal = 0;


void UnprotectedProcess::VThreadProc(void)
{
	DWORD dwCount = 0;
	while (dwCount < m_MaxLoops)
	{
		++dwCount;
		++s_UnprotectedTotal;
	}
	Succeed();
}


int g_ThreadCount = 50;
DWORD g_ThreadLoops = 100000;



void TestThreading(ProcessManager* procMgr, bool isProtected)
{
	int i;
	for (i = 0; i < g_ThreadCount; i++)
	{
		if (isProtected)
		{
			shared_ptr<Process> proc(New ProtectedProcess(g_ThreadLoops));
			procMgr->AttachProcess(proc);
		}
		else
		{
			shared_ptr<Process> proc(New UnprotectedProcess(g_ThreadLoops));
			procMgr->AttachProcess(proc);
		}
	}
	return;
}







class EventSenderProcess : public RealtimeProcess
{
public:
	DWORD m_MaxLoops;
	EventSenderProcess(DWORD maxLoops);
	virtual void VThreadProc();
};



void EventSenderProcess::VThreadProc(void)
{
	DWORD dwCount = 0;

	while (dwCount < m_MaxLoops)
	{
		IEventDataPtr e(New EvtData_Update_Tick(timeGetTime()));
		IEventManager::Get()->VThreadSafeQueueEvent(e);
		Sleep(10);
		dwCount++;
	}

	Succeed();
	InterlockedDecrement(&g_NumRemainingThreads);
}


class EventReaderProcess : public RealtimeProcess
{
public:
	EventReaderProcess() : RealtimeProcess()
	{
		IEventManager::Get()->VAddListener(MakeDelegate(this, &EventReaderProcess::UpdateTickDelegate), EvtData_Update_Tick::s_EventType);
		m_EventsRead = 0;
	}
	void UpdateTickDelegate(IEventDataPtr pEventData);
	virtual void VThreadProc(void);

	ThreadSafeEventQueue m_RealtimeEventQueue;
	int m_EventsRead;
};


void EventReaderProcess::UpdateTickDelegate(IEventDataPtr pEventData)
{
	m_RealtimeEventQueue.Push(pEventData);
}


void EventReaderProcess::VThreadProc(void)
{

	while (true)
	{
		IEventDataPtr e;
		if (m_RealtimeEventQueue.Pop(e))
			++m_EventsRead;
		else
		{
			InterlockedIncrement(&g_NumRemainingThreads);
			if (InterlockedDecrement(&g_NumRemainingThreads) == 0)
				break;
		}
	}


	Succeed();
}


void TestRealtimeEvents(ProcessManager* procMgr)
{
	for (int i = 0; i < THREADS_COUNT; ++i)
	{
		shared_ptr<Process> proc(New EventSenderProcess(g_ThreadLoops));
		procMgr->AttachProcess(proc);
	}
	shared_ptr<Process> proc(New EventReaderProcess());
	procMgr->AttachProcess(proc);
}



class DecompressionProcess : public RealtimeProcess
{

public:
	static void Callback(int progress, bool &cancel);

	DecompressionProcess();
	~DecompressionProcess();

	ThreadSafeEventQueue m_RealtimeEventQueue;

	// Event delegates
	void DecompressRequestDelegate(IEventDataPtr pEventData)
	{
		IEventDataPtr pEventClone = pEventData->VCopy();
		m_RealtimeEventQueue.Push(pEventClone);
	}

	virtual void VThreadProc(void);

};


DecompressionProcess::DecompressionProcess()
	: RealtimeProcess()
{
	IEventManager::Get()->VAddListener(MakeDelegate(this, &DecompressionProcess::DecompressRequestDelegate), EvtData_Decompress_Request::s_EventType);
}

DecompressionProcess::~DecompressionProcess(void)
{
	IEventManager::Get()->VRemoveListener(MakeDelegate(this, &DecompressionProcess::DecompressRequestDelegate), EvtData_Decompress_Request::s_EventType);
}


void DecompressionProcess::VThreadProc()
{
	while (1)
	{
		IEventDataPtr e;
		if (m_RealtimeEventQueue.Pop(e))
		{
			if (EvtData_Decompress_Request::s_EventType == e->VGetEventType())
			{
				shared_ptr<EvtData_Decompress_Request> decomp = static_pointer_cast<EvtData_Decompress_Request>(e);

				ZipFile zipFile;

				bool success = false;

				if (zipFile.Init(decomp->GetZipFileName().c_str()))
				{
					int size = 0;
					int resourceNum = zipFile.Find(decomp->GetFileName().c_str());
					if (resourceNum >= 0)
					{

						char *buffer = New char[size];
						zipFile.ReadFile(resourceNum, buffer);
					}
				}
			}
		}
		else
		{
			Sleep(10);
		}
	}

	Succeed();
}


void TestRealtimeDecompression(ProcessManager *procMgr)
{
	static void *buffer = NULL;

	shared_ptr<Process> proc(New DecompressionProcess());
	procMgr->AttachProcess(proc);

	shared_ptr<EvtData_Decompress_Request> pEventData(New EvtData_Decompress_Request(L"big.zip", "big.dat"));
	IEventManager::Get()->VQueueEvent(pEventData);
}



