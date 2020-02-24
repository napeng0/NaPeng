#include"GameCodeStd.h"
#include"LuaStateManager.h"
#include"Utilities\String.h"

#pragma comment(lib, "luaplus51-1201.lib")

LuaStateManager* LuaStateManager::s_pLuaStateManager = NULL;

bool LuaStateManager::Create()
{
	if (s_pLuaStateManager)
	{
		ERROR("Overwriting LuaStateManager singleton");
		SAFE_DELETE(s_pLuaStateManager);
	}

	s_pLuaStateManager = New LuaStateManager;
	if (s_pLuaStateManager)
		return s_pLuaStateManager->VInit();
	return false;
}

void LuaStateManager::Destroy()
{
	ASSERT(s_pLuaStateManager);
	SAFE_DELETE(s_pLuaStateManager);
}

LuaStateManager::LuaStateManager()
{
	m_pLuaState = NULL;
}


bool LuaStateManager::VInit()
{
	m_pLuaState = LuaPlus::LuaState::Create(true);
	if (m_pLuaState == NULL)
		return false;
	m_pLuaState->GetGlobals().RegisterDirect("ExecuteFile", *this, &LuaStateManager::VExecuteFile);
	m_pLuaState->GetGlobals().RegisterDirect("ExecuteString", *this, &LuaStateManager::VExecuteString);

	return true;
}


void LuaStateManager::VExecuteFile(const char* path)
{
	int result = m_pLuaState->DoFile(path);
	if (result != 0)
		SetError(result);
}


void LuaStateManager::VExecuteString(const char* str)
{
	int result = 0;
	if (strlen(str) <= 1 || str[0] != '=')
	{
		result = m_pLuaState->DoString(str);
		if (result != 0)
			SetError(result);
	}
	else
	{
		std::string buffer("printe(");
		buffer += (str + 1);
		buffer += ")";
		result = m_pLuaState->DoString(buffer.c_str());
		if (result != 0)
			SetError(result);
	}
}


void LuaStateManager::SetError(int error)
{
	LuaPlus::LuaStackObject stackObj(m_pLuaState, -1);
	const char* errStr = stackObj.GetString();
	if (errStr)
	{
		m_LastError = errStr;
		ClearStack();
	}
	else
		m_LastError = "Unknown Lua error";
		
		ERROR(m_LastError);
}

void LuaStateManager::ClearStack()
{
	m_pLuaState->SetTop(0);
}

LuaPlus::LuaObject LuaStateManager::GetGlobalVars()
{
	ASSERT(m_pLuaState);
	return m_pLuaState->GetGlobals();
}


LuaPlus::LuaState* LuaStateManager::GetLuaState() const
{
	return m_pLuaState;
}


LuaPlus::LuaObject LuaStateManager::CreatePath(const char* path, bool ignoreLastElement)
{
	StringVec splitPath;
	Split(path, splitPath, '.');
	if (ignoreLastElement)
		splitPath.pop_back();

	LuaPlus::LuaObject context = GetGlobalVars();
	for (StringVec::iterator it = splitPath.begin(); it != splitPath.end(); ++it)
	{
		if (context.IsNil())
		{
			ERROR("Lua context is Nil");
			return context;
		}
		const std::string& element = *it;
		LuaPlus::LuaObject curr = context.GetByName(element.c_str());

		if(!curr.IsTable())
		{
			if (!curr.IsNil())
			{
				WARNING("Overwriting element " + element + " in table");
				context.SetNil(element.c_str());
			}
			context.CreateTable(element.c_str());

		}
		context = context.GetByName(element.c_str());
	}

	return context;
}


void LuaStateManager::ConvertVec3ToTable(const Vec3& vec, LuaPlus::LuaObject& table) const
{
	table.AssignNewTable(GetLuaState());
	
	table.SetNumber("x", vec.x);
	table.SetNumber("y", vec.y);
	table.SetNumber("z", vec.z);
	
}


void LuaStateManager::ConvertTableToVec3(const LuaPlus::LuaObject& table, Vec3& vec) const
{
	LuaPlus::LuaObject temp;

	temp = table.Get("x");
	if (temp.IsNumber())
		vec.x = temp.GetFloat();
	else
		ERROR("Lua table.x is not a number");

	temp = table.Get("y");
	if (temp.IsNumber())
		vec.y = temp.GetFloat();
	else
		ERROR("Lua table.y is not a number");
	
	temp = table.Get("z");
	if (temp.IsNumber())
		vec.z = temp.GetFloat();
	else
		ERROR("Lua table.z is not a number");
}






