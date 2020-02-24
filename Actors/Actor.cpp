#include"GameCodeStd.h"
#include"Actor.h"
#include"ActorComponent.h"
#include"Utilities\String.h"


Actor::Actor(ActorId id)
{
	m_Id = id;
	m_Type = "Unknown";
	m_Resource = "Unknown";

}

Actor::~Actor()
{
	LOG("Actor", "Destory Actor" + ToStr(m_Id));
	ASSERT(m_Components.empty());

}

bool Actor::Init(TiXmlElement* pData)
{
	LOG("Actor", "Initializing Actor" + ToStr(m_Id));
	m_Type = pData->Attribute("type");
	m_Resource = pData->Attribute("resource");
	return true;
}

void Actor::PostInit()
{
	for (ActorComponents::iterator it = m_Components.begin(); it != m_Components.end(); ++it)
	{
		it->second->VPostInit();
	}
}


void Actor::Destroy()
{
	m_Components.clear();
}

void Actor::Update(int deltaMs)
{
	for (ActorComponents::iterator it = m_Components.begin(); it != m_Components.end(); ++it)
	{
		it->second->VUpdate(deltaMs);
	}
}

std::string Actor::ToXML()
{
	TiXmlDocument outDoc;

	TiXmlElement* pActorElem = New TiXmlElement("Actor");
	pActorElem->SetAttribute("type", m_Type.c_str());
	pActorElem->SetAttribute("resource", m_Resource.c_str());

	//Components
	for (ActorComponents::iterator it = m_Components.begin(); it != m_Components.end(); ++it)
	{
		StrongActorComponentPtr pComp = it->second;
		TiXmlElement* pCompElem = pComp->VGenerateXml();
		pActorElem->LinkEndChild(pCompElem);
	}

	outDoc.LinkEndChild(pActorElem);
	TiXmlPrinter printer;
	outDoc.Accept(&printer);

	return printer.CStr();
}


void Actor::AddComponent(StrongActorComponentPtr pComponent)
{
	std::pair<ActorComponents::iterator, bool> success = m_Components.insert(std::make_pair(pComponent->VGetId(), pComponent));
	ASSERT(success.second);
}