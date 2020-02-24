#include"GameCodeStd.h"
#include"ProcessManager.h"

ProcessManager::~ProcessManager()
{
	ClearAllProcesses();
}

void ProcessManager::ClearAllProcesses()
{
	m_ProcessList.clear();
}

//Update function update every Process in Processlist and 
//returns the number of succeeded process chains in the upper 16 bits and
//the number of failed\aborted proceess chains in the lower 16 bits
unsigned int ProcessManager::UpdateProcesses(unsigned long deltaMs)
{
	unsigned short int successCount = 0;
	unsigned short int failCount = 0;

	ProcessList::iterator it = m_ProcessList.begin();
	while (it != m_ProcessList.end())
	{
		StrongProcessPtr pCur = *it;
		ProcessList::iterator pThis = it;
		++it;

		//If Process is uninitialized,initialize it
		if (pCur->GetState() == Process::UNINITIALIZED)
			pCur->VOnInit();

		//If Process is runnning, update it
		if (pCur->GetState() == Process::RUNNING)
			pCur->VOnUpdate(deltaMs);

		//Destroy a process if it's dead
		if (pCur->IsDead())
		{
			switch (pCur->GetState())
			{
				case Process::SUCCEEDED:
				{
					pCur->VOnSuccess();
					StrongProcessPtr pChild = pCur->RemoveChild();
					if (pChild)
						AttachProcess(pChild);
					else
						++successCount;
					break;
				}
				case Process::FAILED:
				{
					pCur->VOnFailed();
					++failCount;
					break;
				}
				case Process::ABORTED:
				{
					pCur->VOnAbort();
					++failCount;
				}
			}
			m_ProcessList.erase(pThis);
		}
	}
	return (((successCount) << 16) | failCount);
}

WeakProcessPtr ProcessManager::AttachProcess(StrongProcessPtr pProc)
{
	m_ProcessList.push_front(pProc);
	return WeakProcessPtr(pProc);
}

void ProcessManager::AbortAllProcesses(bool isImmediate)
{
	ProcessList::iterator it = m_ProcessList.begin();
	while (it != m_ProcessList.end())
	{
		ProcessList::iterator pthis = it;
		++it;

		StrongProcessPtr pProc = *pthis;
		if (pProc->IsAlive())
		{
			pProc->SetState(Process::ABORTED);
			if (isImmediate)
			{
				pProc->VOnAbort();
				m_ProcessList.erase(pthis);
			}
		}
	}
}