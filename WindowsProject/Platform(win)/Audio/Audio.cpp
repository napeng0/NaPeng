#include"GameCodeStd.h"
#include"Audio.h"
#include"GameCodeStd.h"
#include"Audio.h"
#include"SoundResource.h"

#pragma comment(lib, "dsound")

Audio *g_pAudio = NULL;
const char *gSoundExtentions[] = { ".mp3", ".wav", ".midi", ".ogg" };


Audio::Audio() :
	m_Initialized(false),
	m_AllPaused(false)
{
}


void Audio::VShutDown()
{
	AudioBufferList::iterator i = m_AllSamples.begin();

	while (i != m_AllSamples.end())
	{
		IAudioBuffer *audioBuffer = (*i);
		audioBuffer->VStop();
		m_AllSamples.pop_front();
	}
}


void Audio::VPauseAllSounds()
{
	AudioBufferList::iterator i;
	AudioBufferList::iterator end;
	for (i = m_AllSamples.begin(), end = m_AllSamples.end(); i != end; ++i)
	{
		IAudioBuffer *audioBuffer = (*i);
		audioBuffer->VPause();
	}

	m_AllPaused = true;
}


void Audio::VResumeAllSounds()
{
	AudioBufferList::iterator i;
	AudioBufferList::iterator end;
	for (i = m_AllSamples.begin(), end = m_AllSamples.end(); i != end; ++i)
	{
		IAudioBuffer *audioBuffer = (*i);
		audioBuffer->VResume();
	}

	m_AllPaused = false;
}


void Audio::VStopAllSounds()
{
	IAudioBuffer *audioBuffer = NULL;

	AudioBufferList::iterator i;
	AudioBufferList::iterator end;
	for (i = m_AllSamples.begin(), end = m_AllSamples.end(); i != end; ++i)
	{
		audioBuffer = (*i);
		audioBuffer->VStop();
	}

	m_AllPaused = false;
}


bool Audio::HasSoundCard(void)
{
	return (g_pAudio && g_pAudio->VActive());
}