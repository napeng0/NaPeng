#include "GameCodeStd.h"
#include "ActorFactory.h"
#include "ResourceCache\XmlResource.h"
#include "ActorComponent.h"
#include "AudioComponent.h"
#include "TransformComponent.h"
#include "RenderComponent.h"
#include "PhysicsComponent.h"
#include "AmmoPickup.h"
#include "HealthPickup.h"
#include "BaseScriptComponent.h"
#include "Utilities\String.h"
#include"PickupInterface.h"




ActorFactory::ActorFactory(void)
{
	m_LastActorId = INVALID_ACTOR_ID;

	m_componentFactory.Register<TransformComponent>(ActorComponent::GetIdFromName(TransformComponent::s_Name));
	m_componentFactory.Register<MeshRenderComponent>(ActorComponent::GetIdFromName(MeshRenderComponent::s_Name));
	m_componentFactory.Register<SphereRenderComponent>(ActorComponent::GetIdFromName(SphereRenderComponent::s_Name));
	m_componentFactory.Register<PhysicsComponent>(ActorComponent::GetIdFromName(PhysicsComponent::s_Name));
	m_componentFactory.Register<TeapotRenderComponent>(ActorComponent::GetIdFromName(TeapotRenderComponent::s_Name));
	m_componentFactory.Register<GridRenderComponent>(ActorComponent::GetIdFromName(GridRenderComponent::s_Name));
	m_componentFactory.Register<LightRenderComponent>(ActorComponent::GetIdFromName(LightRenderComponent::s_Name));
	m_componentFactory.Register<SkyRenderComponent>(ActorComponent::GetIdFromName(SkyRenderComponent::s_Name));
	m_componentFactory.Register<AudioComponent>(ActorComponent::GetIdFromName(AudioComponent::s_Name));

	m_componentFactory.Register<AmmoPickup>(ActorComponent::GetIdFromName(AmmoPickup::s_Name));
	m_componentFactory.Register<HealthPickup>(ActorComponent::GetIdFromName(HealthPickup::s_Name));
	m_componentFactory.Register<BaseScriptComponent>(ActorComponent::GetIdFromName(BaseScriptComponent::s_Name));
}

StrongActorPtr ActorFactory::CreateActor(const char* actorResource, TiXmlElement *overrides, const Mat4x4 *pInitialTransform, const ActorId serversActorId)
{
	// Grab the root XML node
	TiXmlElement* pRoot = XmlResourceLoader::LoadAndReturnRootXmlElement(actorResource);
	if (!pRoot)
	{
		ERROR("Failed to create actor from resource: " + std::string(actorResource));
		return StrongActorPtr();
	}

	// Create the actor instance
	ActorId nextActorId = serversActorId;
	if (nextActorId == INVALID_ACTOR_ID)
	{
		nextActorId = GetNextActorId();
	}
	StrongActorPtr pActor(New Actor(nextActorId));
	if (!pActor->Init(pRoot))
	{
		ERROR("Failed to initialize actor: " + std::string(actorResource));
		return StrongActorPtr();
	}

	bool initialTransformSet = false;

	// Loop through each child element and load the component
	for (TiXmlElement* pNode = pRoot->FirstChildElement(); pNode; pNode = pNode->NextSiblingElement())
	{
		StrongActorComponentPtr pComponent(VCreateComponent(pNode));
		if (pComponent)
		{
			pActor->AddComponent(pComponent);
			pComponent->SetOwner(pActor);
		}
		else
		{
			
			return StrongActorPtr();
		}
	}

	if (overrides)
	{
		ModifyActor(pActor, overrides);
	}


	shared_ptr<TransformComponent> pTransformComponent = MakeStrongPtr(pActor->GetComponent<TransformComponent>(TransformComponent::s_Name));
	if (pInitialTransform && pTransformComponent)
	{
		pTransformComponent->SetPosition(pInitialTransform->GetPosition());
	}

	// Now that the actor has been fully created, run the post init phase
	pActor->PostInit();

	return pActor;
}

StrongActorComponentPtr ActorFactory::VCreateComponent(TiXmlElement* pData)
{
	const char* name = pData->Value();
	StrongActorComponentPtr pComponent(m_componentFactory.Create(ActorComponent::GetIdFromName(name)));

	// Initialize the component if we found one
	if (pComponent)
	{
		if (!pComponent->VInit(pData))
		{
			ERROR("Component failed to initialize: " + std::string(name));
			return StrongActorComponentPtr();
		}
	}
	else
	{
		ERROR("Couldn't find ActorComponent named " + std::string(name));
		return StrongActorComponentPtr();  // fail
	}

	// pComponent will be NULL if the component wasn't found. 
	return pComponent;
}


void ActorFactory::ModifyActor(StrongActorPtr pActor, TiXmlElement* overrides)
{
	// Loop through each child element and load the component
	for (TiXmlElement* pNode = overrides->FirstChildElement(); pNode; pNode = pNode->NextSiblingElement())
	{
		ComponentId componentId = ActorComponent::GetIdFromName(pNode->Value());
		StrongActorComponentPtr pComponent = MakeStrongPtr(pActor->GetComponent<ActorComponent>(componentId));
		if (pComponent)
		{
			pComponent->VInit(pNode);

			pComponent->VOnChanged();
		}
		else
		{
			pComponent = VCreateComponent(pNode);
			if (pComponent)
			{
				pActor->AddComponent(pComponent);
				pComponent->SetOwner(pActor);
			}
		}
	}
}