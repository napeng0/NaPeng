#pragma once
#include"GameCodeStd.h"
#include"GameCode\interface.h"

class TiXmlElement;
typedef std::string ActorType;

class Actor
{
	friend class ActorFactory;
public:
	typedef std::map<ComponentId, StrongActorComponentPtr> ActorComponents;

private:
	ActorId m_Id;
	ActorComponents m_Components;
	ActorType m_Type;

	std::string m_Resource;

public:
	explicit Actor(ActorId id);
	~Actor();

	bool Init(TiXmlElement* pData);
	void PostInit();
	void Destroy();
	void Update(int deltaMs);

	std::string ToXML();

	//Accessors
	ActorId GetActorId() const { return m_Id; }
	ActorType GetType() const { return m_Type; }

	//Components retrieving function
	template<class ComponentType>
	weak_ptr<ComponentType> GetComponent(ComponentId id)
	{
		ActorComponents::iterator it = m_Components.find(id);
		if (it != m_Components.end())
		{
			StrongActorComponentPtr pBase(it->second);
			shared_ptr<ComponentType> pSub(static_pointer_cast<ComponentType>(pBase));
			weak_ptr<ComponentType> pWeakSub(pSub);
			return pWeakSub;
		}
		else
		{
			return weak_ptr<ComponentType>();
		}
	}

	template<class ComponentType>
	weak_ptr<ComponentType> GetComponent(const char* name)
	{
		ComponentId id = ActorComponent::GetIdFromName(name);
		ActorComponents::iterator it = m_Components.find(id);
		if (it != m_Components.end())
		{
			StrongActorComponentPtr pBase(it->second);
			shared_ptr<ComponentType> pSub(static_pointer_cast<ComponentType>(pBase));
			weak_ptr<ComponentType> pWeakSub(pSub);
			return pWeakSub;
		}
		else
		{
			return weak_ptr<ComponentType>();
		}
	}

	const ActorComponents* GetComponents() { return &m_Components; }

	void AddComponent(StrongActorComponentPtr pComponent);
};

