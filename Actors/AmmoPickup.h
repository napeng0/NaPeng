#pragma once
#include"PickupInterface.h"

class AmmoPickup : public PickupInterface
{
public:
	static const char* s_Name;
	virtual const char* VGetName() const { return s_Name; }

	virtual bool VInit(TiXmlElement* pData) override;
	virtual TiXmlElement* VGenerateXml(void) override;
	virtual void VApply(WeakActorPtr pActor) override;
};
