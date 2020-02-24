#pragma once
#include"GameCode\interface.h"
#include"3rdParty\Source\GCC4\3rdParty\luaplus51-all\Src\LuaPlus\LuaPlus.h"

class LuaStateManager : public IScriptManager
{
	static LuaStateManager* s_pLuaStateManager;//Singleton
	LuaPlus::LuaState* m_pLuaState;
	std::string m_LastError;

public:
	static bool Create();
	static void Destroy();
	static LuaStateManager* Get()
	{
		ASSERT(s_pLuaStateManager);
		return s_pLuaStateManager;
	}

	virtual bool VInit() override;
	virtual void VExecuteFile(const char* resource) override;
	virtual void VExecuteString(const char* str) override;

	LuaPlus::LuaObject GetGlobalVars();
	LuaPlus::LuaState* GetLuaState() const;

	LuaPlus::LuaObject CreatePath(const char* pathString, bool ignoreLastElement= false);
	void ConvertVec3ToTable(const Vec3& vec, LuaPlus::LuaObject& table) const;
	void ConvertTableToVec3(const LuaPlus::LuaObject& table, Vec3& vec) const;

private:
	void SetError(int error);
	void ClearStack();

	explicit LuaStateManager();
	virtual ~LuaStateManager();

};
