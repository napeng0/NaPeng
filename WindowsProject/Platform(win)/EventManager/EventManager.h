#pragma once
#include<strstream>
#include"MultiCore\CriticalSection.h"
#include"3rdParty\Source\GCC4\3rdParty\FastDelegate\FastDelegate.h"
#include"Debugging\MemoryWatcher.h"


class IEventData;
typedef unsigned long EventType;
typedef shared_ptr<IEventData> IEventDataPtr;
typedef fastdelegate::FastDelegate1<IEventDataPtr> EventListenerDelegate;
typedef ConcurrentQueue<IEventDataPtr> ThreadSafeEventQueue;

extern GenericObjectFactory<IEventData, EventType> g_EventFactory;

#define REGISTER_EVENT(eventClass) g_EventFactory.Register<eventClass>(eventClass::s_EventType)
#define CREATE_EVENT(eventType) g_EventFactory.Create(eventType)

class IEventData
{
public:
	virtual ~IEventData() {};
	virtual const EventType& VGetEventType() const = 0;
	virtual float VGetTimeStamp() const = 0;
	virtual void VSerialize(std::ostrstream& out) const = 0;
	virtual void VDeserialize(std::istrstream& in) = 0;
	virtual IEventDataPtr VCopy() const = 0;
	virtual const char* VGetName() const = 0;


};

class BaseEventData : public IEventData
{
	const float m_TimeStamp;
	
public:
	explicit BaseEventData(const float timeStamp = 0.0f) :m_TimeStamp(timeStamp) {};

	//Accessors
	float VGetTimeStamp() const { return m_TimeStamp; }

	//Serializing for network input/output
	virtual void VSerialize(std::ostrstream& out) const {};
	virtual void VDeserialize(std::istrstream& in) {};

};

class IEventManager
{
public:

	enum Constants {LINFINITE= 0xffffffff};

	explicit IEventManager(const char* name, bool setAsGloabal);
	virtual ~IEventManager();

	virtual bool VAddListener(const EventListenerDelegate& eventDelegate, const EventType& type) = 0;
	
	//Return false if Listner is not found
	virtual bool VRemoveListener(const EventListenerDelegate& eventDelegate, const EventType& type) = 0;
	
	//Fire off event immedately and calls all delegate functions registered for the event
	virtual bool VTriggerEvent(const IEventDataPtr& pEvent) const = 0;
	
	//Use the queue and call the delegate functions 
	virtual bool VThreadSafeQueueEvent(const IEventDataPtr& pEvent) = 0;
	virtual bool VQueueEvent(const IEventDataPtr& pEvent) = 0;
	
	//Find next instance of named event type in event queue and remove it
	virtual bool VAbortEvent(const EventType& type, bool allOfType = false) = 0;

	//Update the status of queued messages, can be used to specify time limits for messages
	//return true if all messages were completed, return false otherwise
	virtual bool VUpdate(unsigned long maxMs = LINFINITE) = 0;

	//Getter for the main gloabal event manager
	static IEventManager* Get();



};

const unsigned int EVENTMANAGER_NUM_QUEUES = 2;

class EventManager : public IEventManager
{
	typedef std::list<EventListenerDelegate> EventListenerList;
	typedef std::map<EventType, EventListenerList> EventListenerMap;
	typedef std::list<IEventDataPtr> EventQueue;

	EventListenerMap m_EventListeners;
	EventQueue m_Queues[EVENTMANAGER_NUM_QUEUES];
	int m_ActiveQueue;//Index of actively proceesing queue, event enque to the opposing queue
	ThreadSafeEventQueue m_RealTimeEventQueue;

public:
	explicit EventManager(const char* name, bool setAsGloabal);
	virtual ~EventManager() {};

	virtual bool VAddListener(const EventListenerDelegate& eventDelegate, const EventType& type);
	virtual bool VRemoveListener(const EventListenerDelegate& eventDelegate, const EventType& type);

	virtual bool VTriggerEvent(const IEventDataPtr& pEvent) const;
	virtual bool VQueueEvent(const IEventDataPtr& pEvent);
	virtual bool VThreadSafeQueueEvent(const IEventDataPtr& pEvent);
	virtual bool VAbortEvent(const EventType& type, bool allOfType = false);

	virtual bool VUpdate(unsigned long maxMs = LINFINITE);




};