#include"GameCodeStd.h"
#include"BaseScriptComponent.h"
#include"LUA\LuaStateManager.h"
#include"Utilities\String.h"
#include"Utilities\Math.h"
#include"TransformComponent.h"
#include"PhysicsComponent.h"
#include"RenderComponent.h"


static const char* METATABLE_NAME = "BaseScriptComponentMetaTable";
const char* BaseScriptComponent::s_Name = "BaseScriptComponent";


BaseScriptComponent::BaseScriptComponent(void)
{
	m_scriptObject.AssignNil(LuaStateManager::Get()->GetLuaState());
	m_scriptDestructor.AssignNil(LuaStateManager::Get()->GetLuaState());
}

BaseScriptComponent::~BaseScriptComponent(void)
{
	// Call the script destructor if there is one
	if (m_scriptDestructor.IsFunction())
	{
		LuaPlus::LuaFunction<void> func(m_scriptDestructor);
		func(m_scriptObject);
	}

	// Clear out the script object
	m_scriptObject.AssignNil(LuaStateManager::Get()->GetLuaState());

	// If we were given a path for this script object, set it to nil
	if (!m_scriptObjectName.empty())
	{
		m_scriptObjectName += " = nil;";
		LuaStateManager::Get()->VExecuteString(m_scriptObjectName.c_str());
	}
}

bool BaseScriptComponent::VInit(TiXmlElement* pData)
{
	LuaStateManager* pStateMgr = LuaStateManager::Get();
	ASSERT(pStateMgr);

	// load the <ScriptObject> tag and validate it
	TiXmlElement* pScriptObjectElement = pData->FirstChildElement("ScriptObject");
	if (!pScriptObjectElement)
	{
		ERROR("No <ScriptObject> tag in XML.  This won't be a very useful script component.");
		return true;  
	}

	// Read all the attributes
	const char* temp = NULL;
	temp = pScriptObjectElement->Attribute("var");
	if (temp)
		m_scriptObjectName = temp;

	temp = pScriptObjectElement->Attribute("constructor");
	if (temp)
		m_constructorName = temp;

	temp = pScriptObjectElement->Attribute("destructor");
	if (temp)
		m_destructorName = temp;

	// Having a var attribute will export the instance of this object to that name.
	if (!m_scriptObjectName.empty())
	{
		m_scriptObject = pStateMgr->CreatePath(m_scriptObjectName.c_str());

		if (!m_scriptObject.IsNil())
		{
			CreateScriptObject();
		}
	}

	
	if (!m_constructorName.empty())
	{
		m_scriptConstructor = pStateMgr->GetGlobalVars().Lookup(m_constructorName.c_str());
		if (m_scriptConstructor.IsFunction())
		{
			// m_scriptObject could be nil if there was no scriptObject attribute.
			if (m_scriptObject.IsNil())
			{
				m_scriptObject.AssignNewTable(pStateMgr->GetLuaState());
				CreateScriptObject();
			}
		}
	}

	// The scriptDestructor attribute is treated as a function in the form of f(scriptObject) and is called
	// when the C++ ScriptObject instance is destroyed.
	if (!m_destructorName.empty())
	{
		m_scriptDestructor = pStateMgr->GetGlobalVars().Lookup(m_destructorName.c_str());
	}

	// Read the <ScriptData> tag
	TiXmlElement* pScriptDataElement = pData->FirstChildElement("ScriptData");
	if (pScriptDataElement)
	{
		if (m_scriptObject.IsNil())
		{
			ERROR("m_scriptObject cannot be nil when ScriptData has been defined");
			return false;
		}

		for (TiXmlAttribute* pAttribute = pScriptDataElement->FirstAttribute(); pAttribute != NULL; pAttribute = pAttribute->Next())
		{
			m_scriptObject.SetString(pAttribute->Name(), pAttribute->Value());
		}
	}

	return true;
}

void BaseScriptComponent::VPostInit(void)
{
	// Call the script constructor if one exists
	if (m_scriptConstructor.IsFunction())
	{
		LuaPlus::LuaFunction<bool> func(m_scriptConstructor);
		func(m_scriptObject);
	}
}

TiXmlElement* BaseScriptComponent::VGenerateXml(void)
{
	TiXmlElement* pBaseElement = New TiXmlElement(VGetName());

	// ScriptObject
	TiXmlElement* pScriptObjectElement = New TiXmlElement("ScriptObject");
	if (!m_scriptObjectName.empty())
		pScriptObjectElement->SetAttribute("var", m_scriptObjectName.c_str());
	if (!m_constructorName.empty())
		pScriptObjectElement->SetAttribute("constructor", m_constructorName.c_str());
	if (!m_destructorName.empty())
		pScriptObjectElement->SetAttribute("destructor", m_destructorName.c_str());
	pBaseElement->LinkEndChild(pScriptObjectElement);

	return pBaseElement;
}

void BaseScriptComponent::CreateScriptObject(void)
{
	LuaStateManager* pStateMgr = LuaStateManager::Get();
	ASSERT(pStateMgr);
	ASSERT(!m_scriptObject.IsNil());

	LuaPlus::LuaObject metaTableObj = pStateMgr->GetGlobalVars().Lookup(METATABLE_NAME);
	ASSERT(!metaTableObj.IsNil());

	LuaPlus::LuaObject boxedPtr = pStateMgr->GetLuaState()->BoxPointer(this);
	boxedPtr.SetMetaTable(metaTableObj);
	m_scriptObject.SetLightUserData("__object", this);
	m_scriptObject.SetMetaTable(metaTableObj);
}

void BaseScriptComponent::RegisterScriptFunctions(void)
{
	// Create the metatable
	LuaPlus::LuaObject metaTableObj = LuaStateManager::Get()->GetGlobalVars().CreateTable(METATABLE_NAME);
	metaTableObj.SetObject("__index", metaTableObj);

	// Transform component functions
	metaTableObj.RegisterObjectDirect("GetActorId", (BaseScriptComponent*)0, &BaseScriptComponent::GetActorId);
	metaTableObj.RegisterObjectDirect("GetPos", (BaseScriptComponent*)0, &BaseScriptComponent::GetPos);
	metaTableObj.RegisterObjectDirect("SetPos", (BaseScriptComponent*)0, &BaseScriptComponent::SetPos);
	metaTableObj.RegisterObjectDirect("GetLookAt", (BaseScriptComponent*)0, &BaseScriptComponent::GetLookAt);
	metaTableObj.RegisterObjectDirect("GetYOrientationRadians", (BaseScriptComponent*)0, &BaseScriptComponent::GetYOrientationRadians);
	metaTableObj.RegisterObjectDirect("RotateY", (BaseScriptComponent*)0, &BaseScriptComponent::RotateY);
	metaTableObj.RegisterObjectDirect("Stop", (BaseScriptComponent*)0, &BaseScriptComponent::Stop);

	metaTableObj.RegisterObjectDirect("SetPosition", (BaseScriptComponent*)0, &BaseScriptComponent::SetPosition);
}

void BaseScriptComponent::UnregisterScriptFunctions(void)
{
	LuaPlus::LuaObject metaTableObj = LuaStateManager::Get()->GetGlobalVars().Lookup(METATABLE_NAME);
	if (!metaTableObj.IsNil())
		metaTableObj.AssignNil(LuaStateManager::Get()->GetLuaState());
}

LuaPlus::LuaObject BaseScriptComponent::GetActorId(void)
{
	
	LuaPlus::LuaObject ret;
	ret.AssignInteger(LuaStateManager::Get()->GetLuaState(), m_pOwner->GetActorId());
	return ret;


}

LuaPlus::LuaObject BaseScriptComponent::GetPos(void)
{
	LuaPlus::LuaObject ret;

	shared_ptr<TransformComponent> pTransformComponent = MakeStrongPtr(m_pOwner->GetComponent<TransformComponent>(TransformComponent::s_Name));
	if (pTransformComponent)
		LuaStateManager::Get()->ConvertVec3ToTable(pTransformComponent->GetPosition(), ret);
	else
		ret.AssignNil(LuaStateManager::Get()->GetLuaState());

	return ret;
}

void BaseScriptComponent::SetPos(LuaPlus::LuaObject newPos)
{
	shared_ptr<TransformComponent> pTransformComponent = MakeStrongPtr(m_pOwner->GetComponent<TransformComponent>(TransformComponent::s_Name));
	if (pTransformComponent)
	{
		Vec3 pos;
		LuaStateManager::Get()->ConvertTableToVec3(newPos, pos);
		pTransformComponent->SetPosition(pos);
	}
	else
	{
		ERROR("Attempting to call SetPos() on an actor with no physcial component; ActorId: " + ToStr(m_pOwner->GetActorId()));
	}
}


LuaPlus::LuaObject BaseScriptComponent::GetLookAt(void) const
{
	LuaPlus::LuaObject ret;

	shared_ptr<TransformComponent> pTransformComponent = MakeStrongPtr(m_pOwner->GetComponent<TransformComponent>(TransformComponent::s_Name));
	if (pTransformComponent)
		LuaStateManager::Get()->ConvertVec3ToTable(pTransformComponent->GetLookAt(), ret);
	else
		ret.AssignNil(LuaStateManager::Get()->GetLuaState());

	return ret;
}

float BaseScriptComponent::GetYOrientationRadians(void) const
{
	shared_ptr<TransformComponent> pTransformComponent = MakeStrongPtr(m_pOwner->GetComponent<TransformComponent>(TransformComponent::s_Name));
	if (pTransformComponent)
	{
		return (GetYRotationFromVector(pTransformComponent->GetLookAt()));
	}
	else
	{
		ERROR("Attempting to call GetYOrientationRadians() on actor with no physical component");
		return 0;
	}
}

void BaseScriptComponent::RotateY(float angleRadians)
{
	shared_ptr<PhysicsComponent> pPhysicalComponent = MakeStrongPtr(m_pOwner->GetComponent<PhysicsComponent>(PhysicsComponent::s_Name));
	if (pPhysicalComponent)
		pPhysicalComponent->RotateY(angleRadians);
}


void BaseScriptComponent::SetPosition(float x, float y, float z)
{
	shared_ptr<PhysicsComponent> pPhysicalComponent = MakeStrongPtr(m_pOwner->GetComponent<PhysicsComponent>(PhysicsComponent::s_Name));
	if (pPhysicalComponent)
		pPhysicalComponent->SetPosition(x, y, z);
}

void BaseScriptComponent::Stop(void)
{
	shared_ptr<PhysicsComponent> pPhysicalComponent = MakeStrongPtr(m_pOwner->GetComponent<PhysicsComponent>(PhysicsComponent::s_Name));
	if (pPhysicalComponent)
		pPhysicalComponent->Stop();
}
