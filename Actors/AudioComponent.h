#pragma once
#include"ActorComponent.h"

class AudioComponent : public ActorComponent
{
	std::string m_AudioResource;
	bool m_looping;
	float m_fadeInTime;
	int m_volume;

public:
	static const char *s_Name;
	virtual const char *VGetName() const { return s_Name; }

	bool Start();

	AudioComponent(void);

	virtual TiXmlElement* VGenerateXml(void);

	// ActorComponent interface
	virtual bool VInit(TiXmlElement* pData) override;
	virtual void VPostInit(void) override;
};