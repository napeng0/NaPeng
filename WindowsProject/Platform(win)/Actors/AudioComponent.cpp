#include"GameCodeStd.h"
#include"Utilities\String.h"
#include"AudioComponent.h"
#include"Audio\SoundProcess.h"
#include"MainLoop\ProcessManager.h"
#include"UI\HumanView.h"



const char* AudioComponent::s_Name = "AudioComponent";


AudioComponent::AudioComponent()
{
	m_AudioResource = "";
	m_looping = false;
	m_fadeInTime = 0.0f;
	m_volume = 100;
}

bool AudioComponent::VInit(TiXmlElement* pData)
{
	TiXmlElement* pTexture = pData->FirstChildElement("Sound");
	if (pTexture)
	{
		m_AudioResource = pTexture->FirstChild()->Value();
	}

	TiXmlElement* pLooping = pData->FirstChildElement("Looping");
	if (pLooping)
	{
		std::string value = pLooping->FirstChild()->Value();
		m_looping = (value == "0") ? false : true;
	}

	TiXmlElement* pFadeIn = pData->FirstChildElement("FadeIn");
	if (pFadeIn)
	{
		std::string value = pFadeIn->FirstChild()->Value();
		m_fadeInTime = (float)atof(value.c_str());
	}

	TiXmlElement* pVolume = pData->FirstChildElement("Volume");
	if (pVolume)
	{
		std::string value = pVolume->FirstChild()->Value();
		m_volume = atoi(value.c_str());
	}

	return true;
}


TiXmlElement* AudioComponent::VGenerateXml(void)
{
	TiXmlElement* pBaseElement = New TiXmlElement(VGetName());

	TiXmlElement* pSoundNode = New TiXmlElement("Sound");
	TiXmlText* pSoundText = New TiXmlText(m_AudioResource.c_str());
	pSoundNode->LinkEndChild(pSoundText);
	pBaseElement->LinkEndChild(pSoundNode);

	TiXmlElement* pLoopingNode = New TiXmlElement("Looping");
	TiXmlText* pLoopingText = New TiXmlText(m_looping ? "1" : "0");
	pLoopingNode->LinkEndChild(pLoopingText);
	pBaseElement->LinkEndChild(pLoopingNode);

	TiXmlElement* pFadeInNode = New TiXmlElement("FadeIn");
	TiXmlText* pFadeInText = New TiXmlText(ToStr(m_fadeInTime).c_str());
	pFadeInNode->LinkEndChild(pFadeInText);
	pBaseElement->LinkEndChild(pFadeInNode);

	TiXmlElement* pVolumeNode = New TiXmlElement("Volume");
	TiXmlText* pVolumeText = New TiXmlText(ToStr(m_volume).c_str());
	pVolumeNode->LinkEndChild(pVolumeText);
	pBaseElement->LinkEndChild(pVolumeNode);

	return pBaseElement;
}

void AudioComponent::VPostInit()
{
	HumanView *humanView = g_pApp->GetHumanView();
	if (!humanView)
	{
		ERROR("Sounds need a human view to be heard!");
		return;
	}

	ProcessManager *processManager = humanView->GetProcessManager();
	if (!processManager)
	{
		ERROR("Sounds need a process manager to attach!");
		return;
	}

	if (!g_pApp->IsEditorRunning())
	{

		Resource resource(m_AudioResource);
		shared_ptr<ResHandle> rh = g_pApp->m_ResCache->GetHandle(&resource);
		shared_ptr<SoundProcess> sound(New SoundProcess(rh, 0, true));
		processManager->AttachProcess(sound);

		// Fade process
		if (m_fadeInTime > 0.0f)
		{
			shared_ptr<FadeProcess> fadeProc(new FadeProcess(sound, 10000, 100));
			processManager->AttachProcess(fadeProc);
		}
	}
}