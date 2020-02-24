#pragma once
#include"GameCodeStd.h"


class Process;
typedef shared_ptr<Process> StrongProcessPtr;
typedef weak_ptr<Process> WeakProcessPtr;

class Process
{
	friend class ProcessManager;

public:
	enum State
	{
		//Processs that are neither dead nor alive
		UNINITIALIZED,//Created but not running
		REMOVED,//Removed from the process list but not destroyed probably because its parent process died
		
		//Living processes
		RUNNING,
		PAUSED,

		//Dead processes
		SUCCEEDED,
		FAILED,
		ABORTED,
	};

private:
	State m_State;
	StrongProcessPtr m_pChild;

private:
	void SetState(State newState) { m_State = newState; }

protected:
	//Interfaces
	virtual void VOnInit() { m_State = RUNNING; }
	virtual void VOnUpdate(unsigned long deltaMs) = 0;
	virtual void VOnSuccess() {};
	virtual void VOnFailed() {};
	virtual void VOnAbort() {};

public:
	Process();
	virtual ~Process();

	//Controllers
	 void Succeed();
	 void Fail();
	 void Pause();
	 void UnPause();

	 //Accessors
	 State GetState() const { return m_State; }
	 bool IsAlive() const { return (m_State == RUNNING || m_State == PAUSED); }
	 bool IsDead() const { return (m_State == SUCCEEDED || m_State == FAILED || m_State == ABORTED); }
	 bool IsRemoved() const { return (m_State == REMOVED); }
	 bool IsPaused() const { return (m_State == PAUSED); }

	 //Child controllers
	 void AttachChild(StrongProcessPtr pChild);
	 StrongProcessPtr RemoveChild();
	 StrongProcessPtr PeekChild() { return m_pChild; }

};


inline void Process::Succeed()
{
	ASSERT(m_State == RUNNING || m_State == PAUSED);
	m_State = SUCCEEDED;
}

inline void Process::Fail()
{
	ASSERT(m_State == RUNNING || m_State == PAUSED);
	m_State = SUCCEEDED;
}

inline void Process::Pause()
{
	if (m_State == RUNNING)
		m_State = PAUSED;
	else
		WARNING("Attempting to pause a process that isn't running");
}

inline void Process::UnPause()
{
	if (m_State == PAUSED)
		m_State = RUNNING;
	else
		WARNING("Attempting to unpause a process that isn's paused");
}

inline void Process::AttachChild(StrongProcessPtr pChild)
{
	if (m_pChild)
		m_pChild->AttachChild(pChild);
	else
		m_pChild = pChild;
}

