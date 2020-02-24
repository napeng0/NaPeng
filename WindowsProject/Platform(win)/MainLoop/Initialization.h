#pragma once
#include"GameCodeStd.h"
#include<winnt.h>


using std::string;

bool CheckStorage(const DWORDLONG diskSpaceNeeded);
DWORD ReadCPUSpeed();
bool CheckMemory(const DWORDLONG physicalRAMNeeded, const DWORDLONG virtualRAMNeeded);
bool IsOnlyInstance(LPCTSTR gameTitle);
const TCHAR* GetSaveGameDirectory(HWND hWnd, const TCHAR* gameAppDirectory);
bool CheckForJoystick(HWND hWnd);

struct GameOptions
{
	string m_Level;
	
	//Rendering options
	string m_Renderer;
	bool m_RunFullSpeed;
	Point m_ScreenSize;

	//Sound options
	float m_SoundEffectsVolume;
	float m_MusicVolume;

	//Multiplayer options
	int m_ExpectedPlayers;
	int m_ListenPort;
	string m_GameHost;
	int m_NumAIs;
	int m_MaxAIs;
	int m_MaxPlayers;

	//Resource option
	bool m_UseDevelopmentDirectories;

	//Extra settings
	TiXmlDocument* m_pDoc;

	GameOptions();
	~GameOptions() { SAFE_DELETE(m_pDoc); }

	void Init(const char* xmlFIlePath, LPWSTR lpCmdLine);
};