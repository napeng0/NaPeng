#pragma once
#include"MainLoop\Process.h"
#include"LuaStateManager.h"


class ScriptProcess : public Process
{
	unsigned long m_Frequency, m_Time;
	LuaPlus::LuaObject m_ScriptInitFunction, m_ScriptUpdateFunction;
	LuaPlus::LuaObject m_ScriptSuccessFunction, m_ScriptFailFunction, m_ScriptAbortFunction;
	LuaPlus::LuaObject m_Self;

public:
	static void RegisterScriptClass(void);

protected:
	// Process interface
	virtual void VOnInit(void);
	virtual void VOnUpdate(unsigned long deltaMs);
	virtual void VOnSuccess(void);
	virtual void VOnFail(void);
	virtual void VOnAbort(void);

private:
	// Private helpers
	static void RegisterScriptClassFunctions(LuaPlus::LuaObject& metaTableObj);
	static LuaPlus::LuaObject CreateFromScript(LuaPlus::LuaObject self, LuaPlus::LuaObject constructionData, LuaPlus::LuaObject originalSubClass);
	virtual bool BuildCppDataFromScript(LuaPlus::LuaObject scriptClass, LuaPlus::LuaObject constructionData);

	// These are needed because the base-class version of these functions are all const and LuaPlus can't deal
	// with registering const functions.
	bool ScriptIsAlive(void) { return IsAlive(); }
	bool ScriptIsDead(void) { return IsDead(); }
	bool ScriptIsPaused(void) { return IsPaused(); }

	// This wrapper function can translate a Lua script object to something C++ can use.
	void ScriptAttachChild(LuaPlus::LuaObject child);

	// Don't allow construction outside of this class
	explicit ScriptProcess(void);
};



