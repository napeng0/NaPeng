#include"GameCodeStd.h"
#include"ScriptExport.h"
#include"ScriptEvent.h"
#include"LuaStateManager.h"
#include"EventManager\Events.h"
#include<set>
#include"MainLoop\Process.h"
#include"Utilities\Math.h"
#include"GameCode\GameCode.h"



class ScriptEventListener
{
	EventType m_EventType;
	LuaPlus::LuaObject m_ScriptCallbackFunction;

public:
	explicit ScriptEventListener(const EventType& eventType, const LuaPlus::LuaObject& scriptCallbackFunction);
	~ScriptEventListener(void);
	EventListenerDelegate GetDelegate(void) { return MakeDelegate(this, &ScriptEventListener::ScriptEventDelegate); }
	void ScriptEventDelegate(IEventDataPtr pEventPtr);
};



class ScriptEventListenerMgr
{
	typedef std::set<ScriptEventListener*> ScriptEventListenerSet;
	ScriptEventListenerSet m_Listeners;

public:
	~ScriptEventListenerMgr(void);
	void AddListener(ScriptEventListener* pListener);
	void DestroyListener(ScriptEventListener* pListener);
};



class InternalScriptExports
{
	static ScriptEventListenerMgr* s_pScriptEventListenerMgr;

public:
	// Initialization
	static bool Init(void);
	static void Destroy(void);

	// Resource loading
	static bool LoadAndExecuteScriptResource(const char* scriptResource);

	// Actors
	static int CreateActor(const char* actorArchetype, LuaPlus::LuaObject luaPosition, LuaPlus::LuaObject luaYawPitchRoll);

	// Event system
	static unsigned long RegisterEventListener(EventType eventType, LuaPlus::LuaObject callbackFunction);
	static void RemoveEventListener(unsigned long listenerId);
	static bool QueueEvent(EventType eventType, LuaPlus::LuaObject eventData);
	static bool TriggerEvent(EventType eventType, LuaPlus::LuaObject eventData);

	// Process system
	static void AttachScriptProcess(LuaPlus::LuaObject scriptProcess);

	// Math
	static float GetYRotationFromVector(LuaPlus::LuaObject vec3);
	static float WrapPi(float wrapMe);
	static LuaPlus::LuaObject GetVectorFromRotation(float angleRadians);

	// Misc
	static void LuaLog(LuaPlus::LuaObject text);
	static unsigned long GetTickCount(void);

	// Physics
	static void ApplyForce(LuaPlus::LuaObject normalDir, float force, int actorId);
	static void ApplyTorque(LuaPlus::LuaObject axis, float force, int actorId);

private:
	static shared_ptr<ScriptEvent> BuildEvent(EventType eventType, LuaPlus::LuaObject& eventData);
};

ScriptEventListenerMgr* InternalScriptExports::s_pScriptEventListenerMgr = NULL;



ScriptEventListenerMgr::~ScriptEventListenerMgr(void)
{
	for (auto it = m_Listeners.begin(); it != m_Listeners.end(); ++it)
	{
		ScriptEventListener* pListener = (*it);
		delete pListener;
	}
	m_Listeners.clear();
}



void ScriptEventListenerMgr::AddListener(ScriptEventListener* pListener)
{
	m_Listeners.insert(pListener);
}



void ScriptEventListenerMgr::DestroyListener(ScriptEventListener* pListener)
{
	ScriptEventListenerSet::iterator findIt = m_Listeners.find(pListener);
	if (findIt != m_Listeners.end())
	{
		m_Listeners.erase(findIt);
		delete pListener;
	}
	else
	{
		ERROR("Couldn't find script listener in set");
	}
}


ScriptEventListener::ScriptEventListener(const EventType& eventType, const LuaPlus::LuaObject& scriptCallbackFunction)
	: m_ScriptCallbackFunction(scriptCallbackFunction)
{
	m_EventType = eventType;
}


ScriptEventListener::~ScriptEventListener(void)
{
	IEventManager* pEventMgr = IEventManager::Get();
	if (pEventMgr)
		pEventMgr->VRemoveListener(GetDelegate(), m_EventType);
}


void ScriptEventListener::ScriptEventDelegate(IEventDataPtr pEvent)
{
	ASSERT(m_ScriptCallbackFunction.IsFunction());  

	// Call the Lua function
	shared_ptr<ScriptEvent> pScriptEvent = static_pointer_cast<ScriptEvent>(pEvent);
	LuaPlus::LuaFunction<void> Callback = m_ScriptCallbackFunction;
	Callback(pScriptEvent->GetEventData());
}



bool InternalScriptExports::Init(void)
{
	ASSERT(s_pScriptEventListenerMgr == NULL);
	s_pScriptEventListenerMgr =New ScriptEventListenerMgr;

	return true;
}


void InternalScriptExports::Destroy(void)
{
	ASSERT(s_pScriptEventListenerMgr != NULL);
	SAFE_DELETE(s_pScriptEventListenerMgr);
}



bool InternalScriptExports::LoadAndExecuteScriptResource(const char* scriptResource)
{
	if (!g_pApp->m_ResCache->IsUsingDevelopmentDirectories())
	{
		Resource resource(std::string(scriptResource));
		shared_ptr<ResHandle> pResourceHandle = g_pApp->m_ResCache->GetHandle(&resource);  
		if (pResourceHandle)
			return true;
		return false;
	}
	else
	{
		
		std::string path("..\\Assets\\");
		path += scriptResource;
		LuaStateManager::Get()->VExecuteFile(path.c_str());
		return true;
	}
}





unsigned long InternalScriptExports::RegisterEventListener(EventType eventType, LuaPlus::LuaObject callbackFunction)
{
	ASSERT(s_pScriptEventListenerMgr);

	if (callbackFunction.IsFunction())
	{
		// Create the C++ listener proxy and set it to listen for the event
		ScriptEventListener* pListener = New ScriptEventListener(eventType, callbackFunction);
		s_pScriptEventListenerMgr->AddListener(pListener);
		IEventManager::Get()->VAddListener(pListener->GetDelegate(), eventType);

		// Convert the pointer to an unsigned long to use as the handle
		unsigned long handle = (unsigned long)(pListener);
		return handle;
	}

	ERROR("Attempting to register script event listener with invalid callback function");
	return 0;
}



void InternalScriptExports::RemoveEventListener(unsigned long listenerId)
{
	ASSERT(s_pScriptEventListenerMgr);
	ASSERT(listenerId != 0);

	// Convert the listenerId back into a pointer
	ScriptEventListener* pListener = reinterpret_cast<ScriptEventListener*>(listenerId);
	s_pScriptEventListenerMgr->DestroyListener(pListener);  
}



bool InternalScriptExports::QueueEvent(EventType eventType, LuaPlus::LuaObject eventData)
{
	shared_ptr<ScriptEvent> pEvent(BuildEvent(eventType, eventData));
	if (pEvent)
	{
		IEventManager::Get()->VQueueEvent(pEvent);
		return true;
	}
	return false;
}



bool InternalScriptExports::TriggerEvent(EventType eventType, LuaPlus::LuaObject eventData)
{
	shared_ptr<ScriptEvent> pEvent(BuildEvent(eventType, eventData));
	if (pEvent)
		return IEventManager::Get()->VTriggerEvent(pEvent);
	return false;
}



shared_ptr<ScriptEvent> InternalScriptExports::BuildEvent(EventType eventType, LuaPlus::LuaObject& eventData)
{
	// Create the event from the event type
	shared_ptr<ScriptEvent> pEvent(ScriptEvent::CreateEventFromScript(eventType));
	if (!pEvent)
		return shared_ptr<ScriptEvent>();

	// Set the event data that was passed in
	if (!pEvent->SetEventData(eventData))
	{
		return shared_ptr<ScriptEvent>();
	}

	return pEvent;
}




void InternalScriptExports::AttachScriptProcess(LuaPlus::LuaObject scriptProcess)
{
	LuaPlus::LuaObject temp = scriptProcess.Lookup("__object");
	if (!temp.IsNil())
	{
		shared_ptr<Process> pProcess((Process*)(temp.GetLightUserData()));
		g_pApp->m_pGame->AttachProcess(pProcess);
	}
	else
	{
		ERROR("Couldn't find __object in script process");
	}
}



int InternalScriptExports::CreateActor(const char* actorArchetype, LuaPlus::LuaObject luaPosition, LuaPlus::LuaObject luaYawPitchRoll)
{

	if (!luaPosition.IsTable())
	{
		ERROR("Invalid object passed to CreateActor(); type = " + std::string(luaPosition.TypeName()));
		return INVALID_ACTOR_ID;
	}

	if (!luaYawPitchRoll.IsTable())
	{
		ERROR("Invalid object passed to CreateActor(); type = " + std::string(luaYawPitchRoll.TypeName()));
		return INVALID_ACTOR_ID;
	}

	Vec3 pos(luaPosition["x"].GetFloat(), luaPosition["y"].GetFloat(), luaPosition["z"].GetFloat());
	Vec3 ypr(luaYawPitchRoll["x"].GetFloat(), luaYawPitchRoll["y"].GetFloat(), luaYawPitchRoll["z"].GetFloat());

	Mat4x4 initialTransform;
	initialTransform.BuildYawPitchRoll(ypr.x, ypr.y, ypr.z);
	initialTransform.SetPosition(pos);

	TiXmlElement *overloads = NULL;
	StrongActorPtr pActor = g_pApp->m_pGame->VCreateActor(actorArchetype, overloads, &initialTransform);

	if (pActor)
	{
		shared_ptr<EvtData_New_Actor> pNewActorEvent(New EvtData_New_Actor(pActor->GetActorId()));
		IEventManager::Get()->VQueueEvent(pNewActorEvent);
		return pActor->GetActorId();
	}

	return INVALID_ACTOR_ID;
}



float InternalScriptExports::WrapPi(float wrapMe)
{
	return WrapPi(wrapMe);
}



float InternalScriptExports::GetYRotationFromVector(LuaPlus::LuaObject vec3)
{
	if (vec3.IsTable())
	{
		Vec3 lookAt(vec3["x"].GetFloat(), vec3["y"].GetFloat(), vec3["z"].GetFloat());
		return ::GetYRotationFromVector(lookAt);
	}

	ERROR("Invalid object passed to GetYRotationFromVector(); type = " + std::string(vec3.TypeName()));
	return 0;
}



LuaPlus::LuaObject InternalScriptExports::GetVectorFromRotation(float angleRadians)
{
	Vec3 result = ::GetVectorFromYRotation(angleRadians);
	LuaPlus::LuaObject luaResult;
	luaResult.AssignNewTable(LuaStateManager::Get()->GetLuaState());
	luaResult.SetNumber("x", result.x);
	luaResult.SetNumber("y", result.y);
	luaResult.SetNumber("z", result.z);
	return luaResult;
}



void InternalScriptExports::LuaLog(LuaPlus::LuaObject text)
{
	if (text.IsConvertibleToString())
	{
		LOG("Lua", text.ToString());
	}
	else
	{
		LOG("Lua", "<" + std::string(text.TypeName()) + ">");
	}
}


unsigned long InternalScriptExports::GetTickCount(void)
{
	return ::GetTickCount();
}



void InternalScriptExports::ApplyForce(LuaPlus::LuaObject normalDirLua, float force, int actorId)
{
	if (normalDirLua.IsTable())
	{
		Vec3 normalDir(normalDirLua["x"].GetFloat(), normalDirLua["y"].GetFloat(), normalDirLua["z"].GetFloat());
		g_pApp->m_pGame->VGetGamePhysics()->VApplyForce(normalDir, force, actorId);
		return;
	}
	ERROR("Invalid object passed to ApplyForce(); type = " + std::string(normalDirLua.TypeName()));
}



void InternalScriptExports::ApplyTorque(LuaPlus::LuaObject axisLua, float force, int actorId)
{
	if (axisLua.IsTable())
	{
		Vec3 axis(axisLua["x"].GetFloat(), axisLua["y"].GetFloat(), axisLua["z"].GetFloat());
		g_pApp->m_pGame->VGetGamePhysics()->VApplyTorque(axis, force, actorId);
		return;
	}
	ERROR("Invalid object passed to ApplyTorque(); type = " + std::string(axisLua.TypeName()));
}



void ScriptExports::Register(void)
{
	LuaPlus::LuaObject globals = LuaStateManager::Get()->GetGlobalVars();

	// Init	
	InternalScriptExports::Init();

	// Resource loading
	globals.RegisterDirect("LoadAndExecuteScriptResource", &InternalScriptExports::LoadAndExecuteScriptResource);

	// Actors
	globals.RegisterDirect("CreateActor", &InternalScriptExports::CreateActor);

	// Event system
	globals.RegisterDirect("RegisterEventListener", &InternalScriptExports::RegisterEventListener);
	globals.RegisterDirect("RemoveEventListener", &InternalScriptExports::RemoveEventListener);
	globals.RegisterDirect("QueueEvent", &InternalScriptExports::QueueEvent);
	globals.RegisterDirect("TriggerEvent", &InternalScriptExports::TriggerEvent);

	// Process system
	globals.RegisterDirect("AttachProcess", &InternalScriptExports::AttachScriptProcess);

	// Math
	LuaPlus::LuaObject mathTable = globals.GetByName("GccMath");
	ASSERT(mathTable.IsTable());
	mathTable.RegisterDirect("GetYRotationFromVector", &InternalScriptExports::GetYRotationFromVector);
	mathTable.RegisterDirect("WrapPi", &InternalScriptExports::WrapPi);
	mathTable.RegisterDirect("GetVectorFromRotation", &InternalScriptExports::GetVectorFromRotation);

	// Misc
	globals.RegisterDirect("Log", &InternalScriptExports::LuaLog);
	globals.RegisterDirect("GetTickCount", &InternalScriptExports::GetTickCount);

	// Physics
	globals.RegisterDirect("ApplyForce", &InternalScriptExports::ApplyForce);
	globals.RegisterDirect("ApplyTorque", &InternalScriptExports::ApplyTorque);
}



void ScriptExports::Unregister(void)
{
	InternalScriptExports::Destroy();
}