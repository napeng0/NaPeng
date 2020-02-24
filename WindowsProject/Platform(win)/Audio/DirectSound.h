#pragma once
#include"Audio.h"
#include<dsound.h>
#include<mmsystem.h>


class DirectSoundAudioBuffer : public AudioBuffer
{
protected:
	LPDIRECTSOUNDBUFFER m_Sample;

public:
	DirectSoundAudioBuffer(LPDIRECTSOUNDBUFFER sample, shared_ptr<ResHandle> resource);
	virtual void *VGet();
	virtual bool VOnRestore();

	virtual bool VPlay(int volume, bool looping);
	virtual bool VPause();
	virtual bool VStop();
	virtual bool VResume();

	virtual bool VTogglePause();
	virtual bool VIsPlaying();
	virtual void VSetVolume(int volume);
	virtual void VSetPosition(unsigned long newPosition);

	virtual float VGetProgress();

private:
	HRESULT FillBufferWithSound();
	HRESULT RestoreBuffer(BOOL* pbWasRestored);
};



class DirectSoundAudio : public Audio
{
public:
	DirectSoundAudio() { m_pDS = NULL; }
	virtual bool VActive() { return m_pDS != NULL; }

	virtual IAudioBuffer *VInitAudioBuffer(shared_ptr<ResHandle> handle);
	virtual void VReleaseAudioBuffer(IAudioBuffer* audioBuffer);

	virtual void VShutDown();
	virtual bool VInitialize(HWND hWnd);

protected:

	IDirectSound8* m_pDS;

	HRESULT SetPrimaryBufferFormat(DWORD dwPrimaryChannels,
		DWORD dwPrimaryFreq,
		DWORD dwPrimaryBitRate);
};