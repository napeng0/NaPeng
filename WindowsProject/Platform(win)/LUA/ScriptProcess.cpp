#include"GameCodeStd.h"
#include"ScriptProcess.h"


const char* SCRIPT_PROCESS_NAME = "ScriptProcess";

ScriptProcess::ScriptProcess(void)
{
	LuaPlus::LuaState* pLuaState = LuaStateManager::Get()->GetLuaState();

	m_Frequency = 0;
	m_Time = 0;
	m_ScriptInitFunction.AssignNil(pLuaState);
	m_ScriptUpdateFunction.AssignNil(pLuaState);
	m_ScriptSuccessFunction.AssignNil(pLuaState);
	m_ScriptFailFunction.AssignNil(pLuaState);
	m_ScriptAbortFunction.AssignNil(pLuaState);
}

bool ScriptProcess::BuildCppDataFromScript(LuaPlus::LuaObject scriptClass, LuaPlus::LuaObject constructionData)
{
	if (scriptClass.IsTable())
	{
		// OnInit()
		LuaPlus::LuaObject temp = scriptClass.GetByName("OnInit");
		if (temp.IsFunction())
			m_ScriptInitFunction = temp;

		// OnUpdate()
		temp = scriptClass.GetByName("OnUpdate");
		if (temp.IsFunction())
		{
			m_ScriptUpdateFunction = temp;
		}
		else
		{
			ERROR("No OnUpdate() found in script process; type == " + std::string(temp.TypeName()));
			return false;  
		}

		// OnSuccess()
		temp = scriptClass.GetByName("OnSuccess");
		if (temp.IsFunction())
			m_ScriptSuccessFunction = temp;

		// OnFail()
		temp = scriptClass.GetByName("OnFail");
		if (temp.IsFunction())
			m_ScriptFailFunction = temp;

		// OnAbort()
		temp = scriptClass.GetByName("OnAbort");
		if (temp.IsFunction())
			m_ScriptAbortFunction = temp;
	}
	else
	{
		ERROR("scriptClass is not a table in ScriptProcess::BuildCppDataFromScript()");
		return false;
	}

	if (constructionData.IsTable())
	{

		for (LuaPlus::LuaTableIterator constructionDataIt(constructionData); constructionDataIt; constructionDataIt.Next())
		{
			const char* key = constructionDataIt.GetKey().GetString();
			LuaPlus::LuaObject val = constructionDataIt.GetValue();

			if (strcmp(key, "frequency") == 0 && val.IsInteger())
				m_Frequency = val.GetInteger();
			else
				m_Self.SetObject(key, val);
		}
	}

	return true;
}

void ScriptProcess::VOnInit(void)
{
	Process::VOnInit();
	if (!m_ScriptInitFunction.IsNil())
	{
		LuaPlus::LuaFunction<void> func(m_ScriptInitFunction);
		func(m_Self);
	}

	
	if (!m_ScriptUpdateFunction.IsFunction())
		Fail();
}

void ScriptProcess::VOnUpdate(unsigned long deltaMs)
{
	m_Time += deltaMs;
	if (m_Time >= m_Frequency)
	{
		LuaPlus::LuaFunction<void> func(m_ScriptUpdateFunction);
		func(m_Self, m_Time);
		m_Time = 0;
	}
}

void ScriptProcess::VOnSuccess(void)
{
	if (!m_ScriptSuccessFunction.IsNil())
	{
		LuaPlus::LuaFunction<void> func(m_ScriptSuccessFunction);
		func(m_Self);
	}
}

void ScriptProcess::VOnFail(void)
{
	if (!m_ScriptFailFunction.IsNil())
	{
		LuaPlus::LuaFunction<void> func(m_ScriptFailFunction);
		func(m_Self);
	}
}

void ScriptProcess::VOnAbort(void)
{
	if (!m_ScriptAbortFunction.IsNil())
	{
		LuaPlus::LuaFunction<void> func(m_ScriptAbortFunction);
		func(m_Self);
	}
}

void ScriptProcess::ScriptAttachChild(LuaPlus::LuaObject child)
{
	if (child.IsTable())
	{
		LuaPlus::LuaObject obj = child.GetByName("__object");
		if (!obj.IsNil())
		{
			// Casting a raw ptr to a smart ptr is generally bad, but Lua has no concept of what a shared_ptr 
			// is.  There's no easy way around it.
			shared_ptr<Process> pProcess((Process*)(obj.GetLightUserData()));
			ASSERT(pProcess);
			AttachChild(pProcess);
		}
		else
		{
			ERROR("Attempting to attach child to ScriptProcess with no valid __object");
		}
	}
	else
	{
		ERROR("Invalid object type passed into ScriptProcess::ScriptAttachChild(); type = " + std::string(child.TypeName()));
	}
}

void ScriptProcess::RegisterScriptClass(void)
{
	LuaPlus::LuaObject metaTableObj = LuaStateManager::Get()->GetGlobalVars().CreateTable(SCRIPT_PROCESS_NAME);
	metaTableObj.SetObject("__index", metaTableObj);
	metaTableObj.SetObject("base", metaTableObj);  // base refers to the parent class; ie the metatable
	metaTableObj.SetBoolean("cpp", true);
	RegisterScriptClassFunctions(metaTableObj);
	metaTableObj.RegisterDirect("Create", &ScriptProcess::CreateFromScript);
}

LuaPlus::LuaObject ScriptProcess::CreateFromScript(LuaPlus::LuaObject self, LuaPlus::LuaObject constructionData, LuaPlus::LuaObject originalSubClass)
{
	
	LOG("Script", std::string("Creating instance of ") + SCRIPT_PROCESS_NAME);
	ScriptProcess* pObj = New ScriptProcess;

	pObj->m_Self.AssignNewTable(LuaStateManager::Get()->GetLuaState());
	if (pObj->BuildCppDataFromScript(originalSubClass, constructionData))
	{
		LuaPlus::LuaObject metaTableObj = LuaStateManager::Get()->GetGlobalVars().Lookup(SCRIPT_PROCESS_NAME);
		ASSERT(!metaTableObj.IsNil());

		pObj->m_Self.SetLightUserData("__object", pObj);
		pObj->m_Self.SetMetaTable(metaTableObj);
	}
	else
	{
		pObj->m_Self.AssignNil(LuaStateManager::Get()->GetLuaState());
		SAFE_DELETE(pObj);
	}

	return pObj->m_Self;
}

void ScriptProcess::RegisterScriptClassFunctions(LuaPlus::LuaObject& metaTableObj)
{
	metaTableObj.RegisterObjectDirect("Succeed", (Process*)0, &Process::Succeed);
	metaTableObj.RegisterObjectDirect("Fail", (Process*)0, &Process::Fail);
	metaTableObj.RegisterObjectDirect("Pause", (Process*)0, &Process::Pause);
	metaTableObj.RegisterObjectDirect("UnPause", (Process*)0, &Process::UnPause);
	metaTableObj.RegisterObjectDirect("IsAlive", (ScriptProcess*)0, &ScriptProcess::ScriptIsAlive);
	metaTableObj.RegisterObjectDirect("IsDead", (ScriptProcess*)0, &ScriptProcess::ScriptIsDead);
	metaTableObj.RegisterObjectDirect("IsPaused", (ScriptProcess*)0, &ScriptProcess::ScriptIsPaused);
	metaTableObj.RegisterObjectDirect("AttachChild", (ScriptProcess*)0, &ScriptProcess::ScriptAttachChild);
}




