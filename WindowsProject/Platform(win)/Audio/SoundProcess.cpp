#include"GameCodeStd.h"
#include"GameCode\GameCode.h"
#include"Audio.h"
#include"SoundProcess.h"
#include"SoundResource.h"


SoundProcess::SoundProcess(shared_ptr<ResHandle> resource, int volume, bool looping) :
	m_Handle(resource),
	m_Volume(volume),
	m_isLooping(looping)
{
	InitializeVolume();
}



SoundProcess::~SoundProcess()
{
	if (IsPlaying())
		Stop();

	if (m_AudioBuffer)
		g_pAudio->VReleaseAudioBuffer(m_AudioBuffer.get());
}


void SoundProcess::InitializeVolume()
{
}


int SoundProcess::GetLengthMilli()
{
	if (m_Handle && m_Handle->GetExtra())
	{
		shared_ptr<SoundResourceExtraData> extra = static_pointer_cast<SoundResourceExtraData>(m_Handle->GetExtra());
		return extra->GetLengthMilli();
	}
	else
	{
		return 0;
	}
}


void SoundProcess::VOnInit()
{
	Process::VOnInit();


	if (m_Handle == NULL || m_Handle->GetExtra() == NULL)
		return;

	
	IAudioBuffer *buffer = g_pAudio->VInitAudioBuffer(m_Handle);

	if (!buffer)
	{
		Fail();
		return;
	}

	m_AudioBuffer.reset(buffer);

	Play(m_Volume, m_isLooping);
}


void SoundProcess::VOnUpdate(unsigned long deltaMs)
{
	if (!IsPlaying())
	{
		Succeed();
	}
}


bool SoundProcess::IsPlaying()
{
	if (!m_Handle || !m_AudioBuffer)
		return false;

	return m_AudioBuffer->VIsPlaying();
}


void SoundProcess::SetVolume(int volume)
{
	if (m_AudioBuffer == NULL)
	{
		return;
	}

	ASSERT(volume >= 0 && volume <= 100 && "Volume must be a number between 0 and 100");
	m_Volume = volume;
	m_AudioBuffer->VSetVolume(volume);
}


int SoundProcess::GetVolume()
{
	if (m_AudioBuffer == NULL)
	{
		return 0;
	}

	m_Volume = m_AudioBuffer->VGetVolume();
	return m_Volume;
}


void SoundProcess::PauseSound()
{
	if (m_AudioBuffer)
		m_AudioBuffer->VTogglePause();
}


void SoundProcess::Play(const int volume, const bool looping)
{
	ASSERT(volume >= 0 && volume <= 100 && "Volume must be a number between 0 and 100");

	if (!m_AudioBuffer)
	{
		return;
	}

	m_AudioBuffer->VPlay(volume, looping);
}


void SoundProcess::Stop()
{
	if (m_AudioBuffer)
	{
		m_AudioBuffer->VStop();
	}
}


float SoundProcess::GetProgress()
{
	if (m_AudioBuffer)
	{
		return m_AudioBuffer->VGetProgress();
	}

	return 0.0f;
}





void ExplosionProcess::VOnInit()
{
	Process::VOnInit();
	Resource resource("explosion.wav");
	shared_ptr<ResHandle> srh = g_pApp->m_ResCache->GetHandle(&resource);
	m_Sound.reset(New SoundProcess(srh));

}


void ExplosionProcess::VOnUpdate(unsigned long deltaMs)
{
	
	float progress = m_Sound->GetProgress();

	switch (m_Stage)
	{
	case 0:
	{
		if (progress > 0.55f)
		{
			++m_Stage;
		}
		break;
	}

	case 1:
	{
		if (progress > 0.75f)
		{
			++m_Stage;
		}
		break;
	}

	default:
	{
		break;
	}
	}
}



FadeProcess::FadeProcess(shared_ptr<SoundProcess> sound, int fadeTime, int endVolume)
{
	m_Sound = sound;
	m_TotalFadeTime = fadeTime;
	m_StartVolume = sound->GetVolume();
	m_EndVolume = endVolume;
	m_ElapsedTime = 0;

	VOnUpdate(0);
}

void FadeProcess::VOnUpdate(unsigned long deltaMs)
{
	m_ElapsedTime += deltaMs;

	if (m_Sound->IsDead())
		Succeed();

	float cooef = (float)m_ElapsedTime / m_TotalFadeTime;
	if (cooef > 1.0f)
		cooef = 1.0f;
	if (cooef < 0.0f)
		cooef = 0.0f;

	int newVolume = m_StartVolume + (int)(float(m_EndVolume - m_StartVolume) * cooef);

	if (m_ElapsedTime >= m_TotalFadeTime)
	{
		newVolume = m_EndVolume;
		Succeed();
	}

	m_Sound->SetVolume(newVolume);
}