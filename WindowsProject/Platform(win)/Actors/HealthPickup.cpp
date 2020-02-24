#include"GameCodeStd.h"
#include"HealthPickup.h"
#include"Utilities\String.h"

const char* HealthPickup::s_Name = "HealthPickup";


bool HealthPickup::VInit(TiXmlElement* pData)
{
	return true;
}

TiXmlElement* HealthPickup::VGenerateXml(void)
{
	TiXmlElement* pComponentElement = New TiXmlElement(VGetName());
	return pComponentElement;
}

void HealthPickup::VApply(WeakActorPtr pActor)
{
	StrongActorPtr pStrongActor = MakeStrongPtr(pActor);
	if (pStrongActor)
	{
		LOG("Actor", "Applying health pickup to actor id " + ToStr(pStrongActor->GetActorId()));
	}
}