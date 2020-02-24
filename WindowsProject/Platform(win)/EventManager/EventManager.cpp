#include"GameCodeStd.h"
#include"EventManager.h"
#include"Utilities\String.h"


static IEventManager* g_pEventMgr = NULL;
GenericObjectFactory<IEventData, EventType> g_EventFactory;

IEventManager* IEventManager::Get()
{
	ASSERT(g_pEventMgr);
	return g_pEventMgr;
}

IEventManager::IEventManager(const char* name, bool setAsGloabl)
{
	if (setAsGloabl)
	{
		if (g_pEventMgr)
		{
			ERROR("Attempting to create two gloabl event managers!");
			delete g_pEventMgr;
		}
		g_pEventMgr = this;
	}
}


IEventManager::~IEventManager()
{
	if (g_pEventMgr == this)
		g_pEventMgr = NULL;
}

EventManager::EventManager(const char* name, bool setAsGlobal) : IEventManager(name, setAsGlobal)
{
	m_ActiveQueue = 0;
}

bool EventManager::VAddListener(const EventListenerDelegate& eventDelegate, const EventType& type)
{
	LOG("Events", "Attempting to add delegate function for event type" + ToStr(type, 16));

	EventListenerList& listenerList = m_EventListeners[type];
	for (EventListenerList::iterator it = listenerList.begin(); it != listenerList.end(); ++it)
	{
		if (eventDelegate == (*it))
		{
			WARNING("Attempting to double-register a delegate");
			return false;
		}
	}

	listenerList.push_back(eventDelegate);
	LOG("Events", "Successfully added delegate for event type:" + ToStr(type, 16));

	return true;

}

bool EventManager::VRemoveListener(const EventListenerDelegate& eventDelegate, const EventType& type)
{
	LOG("Events", "Attempting to remove delegate function from event type:" + ToStr(type, 16));
	bool success = false;
	EventListenerMap::iterator it = m_EventListeners.find(type);
	if (it != m_EventListeners.end())
	{
		EventListenerList listeners = it->second;
		for (EventListenerList::iterator plistener = listeners.begin(); plistener != listeners.end(); ++plistener)
		{
			if (eventDelegate == (*plistener))
			{
				listeners.erase(plistener);
				LOG("Events", "Successfully removed delegate function from event type" + ToStr(type, 16));
				success = true;
				break;
			}
		}
	}
	return success;
}

bool EventManager::VTriggerEvent(const IEventDataPtr& pEvent) const
{
	LOG("Events", "Attempting to trigger event" + std::string(pEvent->VGetName()));
	bool processed = false;

	auto it = m_EventListeners.find(pEvent->VGetEventType());
	if (it != m_EventListeners.end())
	{
		const EventListenerList& listeners = it->second;
		for (EventListenerList::const_iterator plistener = listeners.begin(); plistener != listeners.end(); ++plistener)
		{
			EventListenerDelegate listener = *plistener;
			LOG("Events", "Sending Event:" + std::string(pEvent->VGetName()) + "to delegate.");
			listener(pEvent);
			processed = true;
		}

	}
	return processed;
}


bool EventManager::VQueueEvent(const IEventDataPtr& pEvent)
{
	ASSERT(m_ActiveQueue >= 0);
	ASSERT(m_ActiveQueue < EVENTMANAGER_NUM_QUEUES);

	if (!pEvent)
	{
		ERROR("Invalid event in VQueueEvent()");
		return false;
	}

	LOG("Event", "Attempting to queue event:" + std::string(pEvent->VGetName()));
	auto it = m_EventListeners.find(pEvent->VGetEventType());
	if (it != m_EventListeners.end())
	{
		m_Queues[m_ActiveQueue].push_back(pEvent);
		LOG("Events", "Successfully queued event:" + std::string(pEvent->VGetName()));
		return true;
	}
	else
	{
		LOG("Events", "Skipping event since there are no delegates registered to receive it:" + std::string(pEvent->VGetName()));
		return false;
	}

}


bool EventManager::VAbortEvent(const EventType& type, bool allOfType)
{
	ASSERT(m_ActiveQueue >= 0);
	ASSERT(m_ActiveQueue < EVENTMANAGER_NUM_QUEUES);

	bool success = false;
	EventListenerMap::iterator it = m_EventListeners.find(type);

	if (it != m_EventListeners.end())
	{
		EventQueue& events = m_Queues[m_ActiveQueue];
		auto event = events.begin();
		while (event != events.end())
		{
			auto thisEvent = event;
			++event;
			if ((*thisEvent)->VGetEventType() == type)
			{
				events.erase(thisEvent);
				success = true;
				if (!allOfType)
					break;
			}
		}
	}
	return success;
}


bool EventManager::VUpdate(unsigned long _maxMs)
{
	unsigned long currMs = GetTickCount();
	unsigned long maxMs = (_maxMs == IEventManager::LINFINITE) ? IEventManager::LINFINITE : currMs + _maxMs;

	IEventDataPtr pEvent;
	while (m_RealTimeEventQueue.Pop(pEvent))
	{
		VQueueEvent(pEvent);
		currMs = GetTickCount();
		if (_maxMs != IEventManager::LINFINITE)
		{
			if (currMs >= maxMs)
			{
				ERROR("Real time event timeout: " + ToStr(pEvent->VGetName()));

			}
		}
	}

	//Swap active queues
	int activeQueue = m_ActiveQueue;
	m_ActiveQueue = (m_ActiveQueue + 1) % EVENTMANAGER_NUM_QUEUES;
	m_Queues[m_ActiveQueue].clear();

	LOG("EventLoop", "Processing Event Queue" + ToStr(activeQueue) + ";" + ToStr((unsigned long)m_Queues[activeQueue].size()) + "events to process");

	//Process the queue
	while (!m_Queues[activeQueue].empty())
	{
		pEvent = m_Queues[activeQueue].front();
		m_Queues[activeQueue].pop_front();
		
		LOG("EventLoop", "Processing Event" + std::string(pEvent->VGetName()));

		const EventType& eventType = pEvent->VGetEventType();

		auto it = m_EventListeners.find(eventType);
		if (it != m_EventListeners.end())
		{
			const EventListenerList& listeners = it->second;
			LOG("EventLoop", "Found" + ToStr((unsigned long)listeners.size())+ "delegates");
			for (auto plistener = listeners.begin(); plistener != listeners.end(); ++plistener)
			{
				EventListenerDelegate listener = *plistener;
				LOG("EventLoop", "Sending event" + std::string(pEvent->VGetName()) + "to delegate");
				listener(pEvent);
			}

		}
		currMs = GetTickCount();
		if (_maxMs != IEventManager::LINFINITE&&currMs >= maxMs)
		{
			LOG("EventLoop", "Aborting event processing; time ran out");
			break;
		}

	}

	bool queueFlushed = m_Queues[activeQueue].empty();
	if (!queueFlushed)
	{
		while (!m_Queues[activeQueue].empty())
		{
			pEvent = m_Queues[activeQueue].back();
			m_Queues[activeQueue].pop_back();
			m_Queues[m_ActiveQueue].push_front(pEvent);
		}
	}

	return queueFlushed;
}