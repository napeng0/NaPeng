#pragma once
#include"ActorComponent.h"

class PickupInterface : public ActorComponent
{
public:

	virtual void VApply(WeakActorPtr pActor) = 0;
};