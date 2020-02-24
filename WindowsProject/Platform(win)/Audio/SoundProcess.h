#pragma once
#include"MainLoop\Process.h"
#include"Audio.h"
#include"SoundResource.h"

class SoundProcess : public Process
{
protected:
	shared_ptr<ResHandle> m_Handle;			// This is the raw sound data
	shared_ptr<IAudioBuffer> m_AudioBuffer;		// Handle to the implementation dependent audio buffer 

	int m_Volume;						// these hold the initial setting until the sound is actually launched.
	bool m_isLooping;

public:
	SoundProcess(shared_ptr<ResHandle> soundResource, int volume = 100, bool looping = false);
	virtual ~SoundProcess();

	void Play(const int volume, const bool looping);
	void Stop();

	void SetVolume(int volume);
	int GetVolume();
	int GetLengthMilli();
	bool IsSoundValid() { return m_Handle != NULL; }
	bool IsPlaying();
	bool IsLooping() { return m_AudioBuffer && m_AudioBuffer->VIsLooping(); }
	float GetProgress();
	void PauseSound(void);

protected:
	virtual void VOnInit();
	virtual void VOnUpdate(unsigned long deltaMs);

	void InitializeVolume();

protected:
	SoundProcess();	 // Disable Default Construction

};





class ExplosionProcess : public Process
{
protected:
	int m_Stage;
	shared_ptr<SoundProcess> m_Sound;

public:
	ExplosionProcess() { m_Stage = 0; }

protected:
	virtual void VOnInit();
	virtual void VOnUpdate(unsigned long deltaMs);
};



class FadeProcess : public Process
{
protected:
	shared_ptr<SoundProcess> m_Sound;

	int m_TotalFadeTime;
	int m_ElapsedTime;
	int m_StartVolume;
	int m_EndVolume;

public:
	FadeProcess(shared_ptr<SoundProcess> sound, int fadeTime, int endVolume);
	virtual void VOnUpdate(unsigned long deltaMs);
};