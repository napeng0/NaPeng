#include "GameCodeStd.h"

#include "Events.h"
#include "Physics\PhysicsEventListener.h"
#include "LUA\ScriptEvent.h"



const EventType EvtData_Environment_Loaded::s_EventType(0xa3814acd);
const EventType EvtData_Remote_Environment_Loaded::s_EventType(0x34c8aff1);
const EventType EvtData_New_Actor::s_EventType(0xe86c7c31);
const EventType EvtData_Move_Actor::s_EventType(0xeeaa0a40);
const EventType EvtData_Destroy_Actor::s_EventType(0x77dd2b3a);
const EventType EvtData_New_Render_Component::s_EventType(0xaf4aff75);
const EventType EvtData_Modified_Render_Component::s_EventType(0x80fe9766);
const EventType EvtData_Request_Start_Game::s_EventType(0x11f2b19d);
const EventType EvtData_Remote_Client::s_EventType(0x301693d5);
const EventType EvtData_Network_Player_Actor_Assignment::s_EventType(0xa7c92f11);
const EventType EvtData_Update_Tick::s_EventType(0xf0f5d183);
const EventType EvtData_Decompress_Request::s_EventType(0xc128a129);
const EventType EvtData_Decompression_Progress::s_EventType(0x68de1f28);
const EventType EvtData_Request_New_Actor::s_EventType(0x40378c64);
const EventType EvtData_Request_Destroy_Actor::s_EventType(0xf5395770);
const EventType EvtData_PlaySound::s_EventType(0x3d8118ee);

bool EvtData_PlaySound::VBuildEventFromScript(void)
{
	if (m_EventData.IsString())
	{
		m_SoundResource = m_EventData.GetString();
		return true;
	}

	return false;
}

void RegisterEngineScriptEvents(void)
{
	REGISTER_SCRIPT_EVENT(EvtData_Request_Destroy_Actor, EvtData_Request_Destroy_Actor::s_EventType);
	REGISTER_SCRIPT_EVENT(EvtData_PhysCollision, EvtData_PhysCollision::s_EventType);
	REGISTER_SCRIPT_EVENT(EvtData_PlaySound, EvtData_PlaySound::s_EventType);
}