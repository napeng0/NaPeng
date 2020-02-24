#include"GameCodeStd.h"
#include"GameCode\GameCode.h"
#include"SoundResource.h"
#include"DirectSound.h"
#include<cguid.h>


bool DirectSoundAudio::VInitialize(HWND hWnd)
{
	if (m_Initialized)
		return true;

	m_Initialized = false;
	m_AllSamples.clear();

	SAFE_RELEASE(m_pDS);

	HRESULT hr;

	// Create IDirectSound using the primary sound device
	if (FAILED(hr = DirectSoundCreate8(NULL, &m_pDS, NULL)))
		return false;

	// Set DirectSound coop level 
	if (FAILED(hr = m_pDS->SetCooperativeLevel(hWnd, DSSCL_PRIORITY)))
		return false;

	if (FAILED(hr = SetPrimaryBufferFormat(8, 44100, 16)))
		return false;

	m_Initialized = true;

	return true;
}


HRESULT DirectSoundAudio::SetPrimaryBufferFormat(
	DWORD dwPrimaryChannels,
	DWORD dwPrimaryFreq,
	DWORD dwPrimaryBitRate)
{
	

	HRESULT             hr;
	LPDIRECTSOUNDBUFFER pDSBPrimary = NULL;

	if (!m_pDS)
		return CO_E_NOTINITIALIZED;

	// Get the primary buffer 
	DSBUFFERDESC dsbd;
	ZeroMemory(&dsbd, sizeof(DSBUFFERDESC));
	dsbd.dwSize = sizeof(DSBUFFERDESC);
	dsbd.dwFlags = DSBCAPS_PRIMARYBUFFER;
	dsbd.dwBufferBytes = 0;
	dsbd.lpwfxFormat = NULL;

	if (FAILED(hr = m_pDS->CreateSoundBuffer(&dsbd, &pDSBPrimary, NULL)))
		return DXUT_ERR(L"CreateSoundBuffer", hr);

	WAVEFORMATEX wfx;
	ZeroMemory(&wfx, sizeof(WAVEFORMATEX));
	wfx.wFormatTag = (WORD)WAVE_FORMAT_PCM;
	wfx.nChannels = (WORD)dwPrimaryChannels;
	wfx.nSamplesPerSec = (DWORD)dwPrimaryFreq;
	wfx.wBitsPerSample = (WORD)dwPrimaryBitRate;
	wfx.nBlockAlign = (WORD)(wfx.wBitsPerSample / 8 * wfx.nChannels);
	wfx.nAvgBytesPerSec = (DWORD)(wfx.nSamplesPerSec * wfx.nBlockAlign);

	if (FAILED(hr = pDSBPrimary->SetFormat(&wfx)))
		return DXUT_ERR(L"SetFormat", hr);

	SAFE_RELEASE(pDSBPrimary);

	return S_OK;
}



void DirectSoundAudio::VShutDown()
{
	if (m_Initialized)
	{
		Audio::VShutDown();
		SAFE_RELEASE(m_pDS);
		m_Initialized = false;
	}
}



IAudioBuffer *DirectSoundAudio::VInitAudioBuffer(shared_ptr<ResHandle> resHandle)//const
{
	shared_ptr<SoundResourceExtraData> extra = static_pointer_cast<SoundResourceExtraData>(resHandle->GetExtra());

	if (!m_pDS)
		return NULL;

	switch (extra->GetSoundType())
	{
	case SOUND_TYPE_OGG:
	case SOUND_TYPE_WAVE:
		// Support WAVs and OGGs
		break;

	case SOUND_TYPE_MP3:
	case SOUND_TYPE_MIDI:	
		ASSERT(false && "MP3s and MIDI are not supported");
		return NULL;
		break;

	default:
		ASSERT(false && "Unknown sound type");
		return NULL;
	}//end switch

	LPDIRECTSOUNDBUFFER sampleHandle;

	// Create the direct sound buffer
	DSBUFFERDESC dsbd;
	ZeroMemory(&dsbd, sizeof(DSBUFFERDESC));
	dsbd.dwSize = sizeof(DSBUFFERDESC);
	dsbd.dwFlags = DSBCAPS_CTRLVOLUME;
	dsbd.dwBufferBytes = resHandle->Size();
	dsbd.guid3DAlgorithm = GUID_NULL;
	dsbd.lpwfxFormat = const_cast<WAVEFORMATEX *>(extra->GetFormat());

	HRESULT hr;
	if (FAILED(hr = m_pDS->CreateSoundBuffer(&dsbd, &sampleHandle, NULL)))
	{
		return NULL;
	}

	// Add handle to the list
	IAudioBuffer *audioBuffer = New DirectSoundAudioBuffer(sampleHandle, resHandle);
	m_AllSamples.push_front(audioBuffer);

	return audioBuffer;

}


void DirectSoundAudio::VReleaseAudioBuffer(IAudioBuffer *sampleHandle)//const
{
	sampleHandle->VStop();
	m_AllSamples.remove(sampleHandle);
}


DirectSoundAudioBuffer::DirectSoundAudioBuffer(
	LPDIRECTSOUNDBUFFER sample,
	shared_ptr<ResHandle> resource)
	: AudioBuffer(resource)
{
	m_Sample = sample;
	FillBufferWithSound();
}

void *DirectSoundAudioBuffer::VGet()
{
	if (!VOnRestore())
		return NULL;

	return m_Sample;
}



bool DirectSoundAudioBuffer::VPlay(int volume, bool looping)
{
	if (!g_pAudio->VActive())
		return false;

	VStop();

	m_Volume = volume;
	m_isLooping = looping;

	LPDIRECTSOUNDBUFFER pDSB = (LPDIRECTSOUNDBUFFER)VGet();
	if (!pDSB)
		return false;

	pDSB->SetVolume(volume);

	DWORD dwFlags = looping ? DSBPLAY_LOOPING : 0L;

	return (S_OK == pDSB->Play(0, 0, dwFlags));

}


bool DirectSoundAudioBuffer::VStop()
{
	if (!g_pAudio->VActive())
		return false;

	LPDIRECTSOUNDBUFFER pDSB = (LPDIRECTSOUNDBUFFER)VGet();

	if (!pDSB)
		return false;

	m_isPaused = true;
	pDSB->Stop();
	return true;
}



bool DirectSoundAudioBuffer::VPause()
{
	LPDIRECTSOUNDBUFFER pDSB = (LPDIRECTSOUNDBUFFER)VGet();

	if (!g_pAudio->VActive())
		return false;

	if (pDSB)
		return false;

	m_isPaused = true;
	pDSB->Stop();
	pDSB->SetCurrentPosition(0);	// Rewinds buffer to beginning.
	return true;
}


bool DirectSoundAudioBuffer::VResume()
{
	m_isPaused = false;
	return VPlay(VGetVolume(), VIsLooping());
}


bool DirectSoundAudioBuffer::VTogglePause()
{
	if (!g_pAudio->VActive())
		return false;

	if (m_isPaused)
	{
		VResume();
	}
	else
	{
		VPause();				
								
	}

	return true;
}






bool DirectSoundAudioBuffer::VIsPlaying()
{
	if (!g_pAudio->VActive())
		return false;

	DWORD dwStatus = 0;
	LPDIRECTSOUNDBUFFER pDSB = (LPDIRECTSOUNDBUFFER)VGet();
	pDSB->GetStatus(&dwStatus);
	bool bIsPlaying = ((dwStatus & DSBSTATUS_PLAYING) != 0);

	return bIsPlaying;
}


void DirectSoundAudioBuffer::VSetVolume(int volume)
{
	
	int myDSBVolumeMin = DSBVOLUME_MIN;

	if (!g_pAudio->VActive())
		return;

	LPDIRECTSOUNDBUFFER pDSB = (LPDIRECTSOUNDBUFFER)VGet();

	ASSERT(volume >= 0 && volume <= 100 && "Volume must be a number between 0 and 100");

	// Convert volume from 0-100 into range for DirectX 

	float coeff = (float)volume / 100.0f;
	float logarithmicProportion = coeff > 0.1f ? 1 + log10(coeff) : 0;
	float range = float(DSBVOLUME_MAX - myDSBVolumeMin);
	float fvolume = (range * logarithmicProportion) + myDSBVolumeMin;

	ASSERT(fvolume >= myDSBVolumeMin && fvolume <= DSBVOLUME_MAX);
	HRESULT hr = pDSB->SetVolume(LONG(fvolume));
	ASSERT(hr == S_OK);

}

void DirectSoundAudioBuffer::VSetPosition(unsigned long newPosition)
{
	m_Sample->SetCurrentPosition(newPosition);
}



bool DirectSoundAudioBuffer::VOnRestore()
{
	HRESULT hr;
	BOOL    bRestored;

	// Restore the buffer if it was lost
	if (FAILED(hr = RestoreBuffer(&bRestored)))
		return NULL;

	if (bRestored)
	{
		// The buffer was restored, so we need to fill it with new data
		if (FAILED(hr = FillBufferWithSound()))
			return NULL;
	}

	return true;
}


HRESULT DirectSoundAudioBuffer::RestoreBuffer(BOOL* pbWasRestored)
{
	HRESULT hr;

	if (!m_Sample)
		return CO_E_NOTINITIALIZED;
	if (pbWasRestored)
		*pbWasRestored = FALSE;

	DWORD dwStatus;
	if (FAILED(hr = m_Sample->GetStatus(&dwStatus)))
		return DXUT_ERR(L"GetStatus", hr);

	if (dwStatus & DSBSTATUS_BUFFERLOST)
	{
		// Since the app could have just been activated, then
		// DirectSound may not be giving us control yet, so 
		// the restoring the buffer may fail.  
		// If it does, sleep until DirectSound gives us control but fail if
		// if it goes on for more than 1 second
		int count = 0;
		do
		{
			hr = m_Sample->Restore();
			if (hr == DSERR_BUFFERLOST)
				Sleep(10);
		} while ((hr = m_Sample->Restore()) == DSERR_BUFFERLOST && ++count < 100);

		if (pbWasRestored != NULL)
			*pbWasRestored = TRUE;

		return S_OK;
	}
	else
	{
		return S_FALSE;
	}
}


HRESULT DirectSoundAudioBuffer::FillBufferWithSound(void)
{
	HRESULT hr;
	VOID	*pDSLockedBuffer = NULL;	 // A pointer to the DirectSound buffer
	DWORD   dwDSLockedBufferSize = 0;    // Size of the locked DirectSound buffer
	DWORD   dwWavDataRead = 0;    // Amount of data read from the wav file 

	if (!m_Sample)
		return CO_E_NOTINITIALIZED;

	if (FAILED(hr = RestoreBuffer(NULL)))
		return DXUT_ERR(L"RestoreBuffer", hr);

	int pcmBufferSize = m_Resource->Size();
	shared_ptr<SoundResourceExtraData> extra = static_pointer_cast<SoundResourceExtraData>(m_Resource->GetExtra());


	// Lock the buffer down
	if (FAILED(hr = m_Sample->Lock(0, pcmBufferSize,
		&pDSLockedBuffer, &dwDSLockedBufferSize,
		NULL, NULL, 0L)))
		return DXUT_ERR(L"Lock", hr);

	if (pcmBufferSize == 0)
	{
		// Wav is blank, so just fill with silence
		FillMemory((BYTE*)pDSLockedBuffer,
			dwDSLockedBufferSize,
			(BYTE)(extra->GetFormat()->wBitsPerSample == 8 ? 128 : 0));
	}
	else
	{
		CopyMemory(pDSLockedBuffer, m_Resource->Buffer(), pcmBufferSize);
		if (pcmBufferSize < (int)dwDSLockedBufferSize)
		{
			// If the buffer sizes are different fill in the rest with silence 
			FillMemory((BYTE*)pDSLockedBuffer + pcmBufferSize,
				dwDSLockedBufferSize - pcmBufferSize,
				(BYTE)(extra->GetFormat()->wBitsPerSample == 8 ? 128 : 0));
		}
	}

	// Unlock the buffer, we don't need it anymore.
	m_Sample->Unlock(pDSLockedBuffer, dwDSLockedBufferSize, NULL, 0);

	return S_OK;
}





float DirectSoundAudioBuffer::VGetProgress()
{
	LPDIRECTSOUNDBUFFER pDSB = (LPDIRECTSOUNDBUFFER)VGet();
	DWORD progress = 0;

	pDSB->GetCurrentPosition(&progress, NULL);

	float length = (float)m_Resource->Size();

	return (float)progress / length;
}