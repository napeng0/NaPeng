#pragma once
#include"Actor.h"


class ActorFactory
{
	ActorId m_LastActorId;

protected:
	GenericObjectFactory<ActorComponent, ComponentId> m_componentFactory;

public:
	ActorFactory(void);

	StrongActorPtr CreateActor(const char* actorResource, TiXmlElement* overrides, const Mat4x4* initialTransform, const ActorId serversActorId);
	void ModifyActor(StrongActorPtr pActor, TiXmlElement* overrides);

	
	virtual StrongActorComponentPtr VCreateComponent(TiXmlElement* pData);

private:
	ActorId GetNextActorId(void) { ++m_LastActorId; return m_LastActorId; }
};