#pragma once
#include"Process.h"

class ProcessManager
{
	typedef std::list<StrongProcessPtr> ProcessList;
	ProcessList m_ProcessList;

	void ClearAllProcesses();

public:
	~ProcessManager();

	//Interfaces
	unsigned int UpdateProcesses(unsigned long delatMs);
	WeakProcessPtr AttachProcess(StrongProcessPtr pProcess);
	void AbortAllProcesses(bool isImmediate);

	//Accessors
	unsigned int GetProcessCount() const { return m_ProcessList.size(); }

};