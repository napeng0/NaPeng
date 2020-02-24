#pragma once
#include"GameCodeStd.h"
#include"ResourceCache\ResCache.h"


class SoundResourceExtraData;




enum SoundType
{
	SOUND_TYPE_FIRST,
	SOUND_TYPE_MP3 = SOUND_TYPE_FIRST,
	SOUND_TYPE_WAVE,
	SOUND_TYPE_MIDI,
	SOUND_TYPE_OGG,
	SOUND_TYPE_COUNT,
	SOUND_TYPE_UNKNOWN,
};

extern char *gSoundExtentions[];



class IAudioBuffer
{
public:
	virtual ~IAudioBuffer() { }

	virtual void *VGet() = 0;
	virtual shared_ptr<ResHandle> VGetResource() = 0;
	virtual bool VOnRestore() = 0;

	virtual bool VPlay(int volume, bool looping) = 0;
	virtual bool VPause() = 0;
	virtual bool VStop() = 0;
	virtual bool VResume() = 0;

	virtual bool VTogglePause() = 0;
	virtual bool VIsPlaying() = 0;
	virtual bool VIsLooping() const = 0;
	virtual void VSetVolume(int volume) = 0;
	virtual void VSetPosition(unsigned long newPosition) = 0;
	virtual int VGetVolume() const = 0;
	virtual float VGetProgress() = 0;
};



class AudioBuffer : public IAudioBuffer
{
public:
	virtual shared_ptr<ResHandle> VGetResource() { return m_Resource; }
	virtual bool VIsLooping() const { return m_isLooping; }
	virtual int VGetVolume() const { return m_Volume; }
protected:
	AudioBuffer(shared_ptr<ResHandle >resource)
	{
		m_Resource = resource;
		m_isPaused = false;
		m_isLooping = false;
		m_Volume = 0;
	}

	shared_ptr<ResHandle> m_Resource;


	bool m_isPaused;


	bool m_isLooping;


	int m_Volume;
};




class IAudio
{
public:
	virtual bool VActive() = 0;

	virtual IAudioBuffer *VInitAudioBuffer(shared_ptr<ResHandle> handle) = 0;
	virtual void VReleaseAudioBuffer(IAudioBuffer* audioBuffer) = 0;

	virtual void VStopAllSounds() = 0;
	virtual void VPauseAllSounds() = 0;
	virtual void VResumeAllSounds() = 0;

	virtual bool VInitialize(HWND hWnd) = 0;
	virtual void VShutDown() = 0;
};



class Audio : public IAudio
{
public:
	Audio();
	~Audio() { VShutDown(); }
	virtual void VStopAllSounds();
	virtual void VPauseAllSounds();
	virtual void VResumeAllSounds();

	virtual void VShutDown();
	static bool HasSoundCard(void);
	bool IsPaused() { return m_AllPaused; }

protected:

	typedef std::list<IAudioBuffer *> AudioBufferList;

	AudioBufferList m_AllSamples;	// List of all currently allocated audio buffers
	bool m_AllPaused;				// Paused?
	bool m_Initialized;				// Initialized?
};

extern Audio *g_pAudio;