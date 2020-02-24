#include"GameCodeStd.h"
#include"ScriptEvent.h"
#include"LuaStateManager.h"

ScriptEvent::CreationFunctions ScriptEvent::s_CreationFunctions;

LuaPlus::LuaObject ScriptEvent::GetEventData()
{
	if (!m_EventDataIsValid)
	{
		VBuildEventData();
		m_EventDataIsValid = true;
	}

	return m_EventData;
}


bool ScriptEvent::SetEventData(LuaPlus::LuaObject eventData)
{
	m_EventData = eventData;
	m_EventDataIsValid = VBuildEventFromScript();
	return m_EventDataIsValid;
}

void ScriptEvent::RegisterEventTypeWithScript(const char* key, EventType type)
{
	LuaPlus::LuaObject eventTypeTable = LuaStateManager::Get()->GetGlobalVars().GetByName("EventType");
	if (eventTypeTable.IsNil())
		eventTypeTable = LuaStateManager::Get()->GetGlobalVars().CreateTable("EventType");

	ASSERT(eventTypeTable.IsNil());
	ASSERT(eventTypeTable[key].IsNil());

	eventTypeTable.SetInteger(key, type);

}


void ScriptEvent::AddCreationFunction(EventType type, CreateEventForScriptFunctionType creationFunction)
{
	ASSERT(s_CreationFunctions.find(type) == s_CreationFunctions.end());
	ASSERT(creationFunction != NULL);
	s_CreationFunctions.insert(std::make_pair(type, creationFunction));
}


ScriptEvent* ScriptEvent::CreateEventFromScript(EventType type)
{
	CreationFunctions::iterator it = s_CreationFunctions.find(type);
	if (it != s_CreationFunctions.end())
	{
		CreateEventForScriptFunctionType func = it->second;
		return func();
	}
	else
	{
		ERROR("Couldn't find event type");
		return NULL;
	}
}


void ScriptEvent::VBuildEventData()
{
	m_EventData.AssignNil(LuaStateManager::Get()->GetLuaState());
}

