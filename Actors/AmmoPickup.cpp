#include"GameCodeStd.h"
#include"AmmoPickup.h"
#include"Utilities\String.h"


const char* AmmoPickup::s_Name = "AmmoPickup";

bool AmmoPickup::VInit(TiXmlElement* pData)
{
	return true;
}

TiXmlElement* AmmoPickup::VGenerateXml(void)
{
	TiXmlElement* pComponentElement = New TiXmlElement(VGetName());
	return pComponentElement;
}

void AmmoPickup::VApply(WeakActorPtr pActor)
{
	StrongActorPtr pStrongActor = MakeStrongPtr(pActor);
	if (pStrongActor)
	{
		LOG("Actor", "Applying ammo pickup to actor id " + ToStr(pStrongActor->GetActorId()));
	}
}


