#pragma once
#include"Utilities\String.h"
#include"GameCode\interface.h"

class TiXmlElement;

class ActorComponent
{
	friend class ActorFactory;

protected:
	StrongActorPtr m_pOwner;

public:
	virtual ~ActorComponent() { m_pOwner.reset(); }

	//Interfaces
	virtual bool VInit(TiXmlElement* pData) = 0;
	virtual void VPostInit() {}
	virtual void VUpdate(int deltaMs) {}
	virtual void VOnChanged() {}

	//For the editor
	virtual TiXmlElement* VGenerateXml() = 0;

	virtual const char* VGetName() const = 0;
	static ComponentId GetIdFromName(const char* name)
	{
		void* rawId = HashedString::HashName(name);
		return reinterpret_cast<ComponentId>(rawId);
	}
	virtual ComponentId VGetId() const { return GetIdFromName(VGetName()); }

private:
	void SetOwner(StrongActorPtr pOwner) { m_pOwner = pOwner; }
};