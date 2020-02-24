#pragma once
#include"LuaPlus.h"
#include"GameCodeStd.h"


class BaseScriptComponent : ActorComponent
{
	std::string m_scriptObjectName;
	std::string m_constructorName;
	std::string m_destructorName;

	LuaPlus::LuaObject m_scriptObject;
	LuaPlus::LuaObject m_scriptConstructor;
	LuaPlus::LuaObject m_scriptDestructor;

public:
	static const char* s_Name;
	virtual const char* VGetName() const { return s_Name; }

	BaseScriptComponent(void);
	virtual ~BaseScriptComponent(void);
	virtual bool VInit(TiXmlElement* pData);
	virtual void VPostInit(void);
	virtual TiXmlElement* VGenerateXml(void);

	static void RegisterScriptFunctions(void);
	static void UnregisterScriptFunctions(void);

private:
	void CreateScriptObject(void);

	// component script functions
	LuaPlus::LuaObject GetActorId(void);

	// physics component script functions
	LuaPlus::LuaObject GetPos(void);
	void SetPos(LuaPlus::LuaObject newPos);
	LuaPlus::LuaObject GetLookAt(void) const;
	float GetYOrientationRadians(void) const;
	void RotateY(float angleRadians);
	void SetPosition(float x, float y, float z);
	void Stop(void);
};