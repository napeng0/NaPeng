#pragma once
#include"EventManager\EventManager.h"
#include"3rdParty\Source\GCC4\3rdParty\luaplus51-all\Src\LuaPlus\LuaPlus.h"

class ScriptEvent;
typedef ScriptEvent* (*CreateEventForScriptFunctionType)();

class ScriptEvent : public BaseEventData
{
	typedef std::map<EventType, CreateEventForScriptFunctionType> CreationFunctions;
	
	static CreationFunctions s_CreationFunctions;
	bool m_EventDataIsValid;

protected:
	LuaPlus::LuaObject m_EventData;

public:
	ScriptEvent() { m_EventDataIsValid = false; }
	//Get event data for script
	LuaPlus::LuaObject GetEventData();
	//Import event from script to C++
	bool SetEventData(LuaPlus::LuaObject eventData);

	static void RegisterEventTypeWithScript(const char* key, EventType type);
	static void AddCreationFunction(EventType type, CreateEventForScriptFunctionType pCreationFunction);
	static ScriptEvent* CreateEventFromScript(EventType type);
	
protected:
	//Build Event data for script function
	virtual void VBuildEventData();

	//Build Event data for C++
	virtual bool VBuildEventFromScript() { return true; }


};

#define REGISTER_SCRIPT_EVENT(eventClass, eventType) \
	ScriptEvent::RegisterEventTypeWithScript(#eventClass, eventType);\
	ScriptEvent::AddCreationFunction(eventType, &eventClass::CreateEventForScript);


#define EXPORT_FOR_SCRIPT_EVENT(eventClass) \
public:\
	static ScriptEvent* CreateEventForScript()\
	{\
		return new eventClass;\
	}

