#include"GameCodeStd.h"
#include"Initialization.h"
#include<ShlObj.h>
#include<direct.h>

bool CheckStorage(const DWORDLONG diskSpaceNeeded)
{
	//Check for enough free disk space on current disk
	const int drive = _getdrive();
	struct _diskfree_t diskfree;
	_getdiskfree(drive, &diskfree);
	const unsigned __int64 neededClusters = diskSpaceNeeded / (diskfree.sectors_per_cluster*diskfree.bytes_per_sector);
	if (diskfree.avail_clusters < neededClusters)
	{
		ERROR("Not enough physical storage.");
		return false;
	}
	return true;

}

bool CheckMemory(const DWORDLONG physicalRAMNeeded, const DWORDLONG virtualRAMNeeded)
{
	MEMORYSTATUSEX status;
	GlobalMemoryStatusEx(&status);
	
	//Check for physical memory
	if (status.ullTotalPhys < physicalRAMNeeded)
	{
		ERROR("Not enough physical memory");
		return false;
	}

	//Check for enough free memory
	if (status.ullAvailVirtual < virtualRAMNeeded)
	{
		ERROR("Not enough virtual memory.");
		return false;
	}

	//Check for enough contiguous memeory
	char* buffer = New char[virtualRAMNeeded];
	if (buffer)
		delete[] buffer;
	else
	{
		ERROR("Not enough contiguous available memory");
		return false;
	}

	return true;

}


DWORD ReadCPUSpeed()
{
	DWORD BufSize = sizeof(DWORD);
	DWORD MHz = 0;
	DWORD type = REG_DWORD;
	HKEY hKey;

	long error = RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 0, KEY_READ, &hKey);
	if (error == ERROR_SUCCESS)
	{
		RegQueryValueEx(hKey, L"~MHz", NULL, &type, (LPBYTE)&MHz, &BufSize);
	}
	return MHz;
}

GameOptions::GameOptions()
{
	m_Level = "";
	m_Renderer = "Direct3D 9";
	m_RunFullSpeed = false;
	m_SoundEffectsVolume = 1.0f;
	m_MusicVolume = 1.0f;
	m_ExpectedPlayers = 1;
	m_ListenPort = -1;
	m_GameHost = "GameHost";
	m_NumAIs = 1;
	m_MaxAIs = 4;
	m_MaxPlayers = 4;
	m_ScreenSize = Point(1920, 1080);
	m_UseDevelopmentDirectories = false;
	m_pDoc = NULL;
}

void GameOptions::Init(const char* xmlFileName, LPWSTR lpCmdLine)
{
	//Command line can overide the XML file option
	m_pDoc = new TiXmlDocument(xmlFileName);
	if (m_pDoc&& m_pDoc->LoadFile())
	{
		TiXmlElement* pRoot = m_pDoc->RootElement();
		if (!pRoot)
			return;

		//Loop through each child element
		TiXmlElement* pNode = NULL;
		pNode = pRoot->FirstChildElement("Graphics");
		if (pNode)
		{
			string attribute;
			attribute = pNode->Attribute("Renderer");
			if (attribute != "Direct3D 9"&&attribute != "Direct3D 11")
				ASSERT(0 && "Bad Renderer setting");
			else
				m_Renderer = attribute;

			if (pNode->Attribute("width"))
			{
				m_ScreenSize.x = atoi(pNode->Attribute("width"));
				if (m_ScreenSize.x < 800)
					m_ScreenSize.x = 800;
			}
			if (pNode->Attribute("Height"))
			{
				m_ScreenSize.y = atoi(pNode->Attribute("Height"));
				if (m_ScreenSize.y < 600)
					m_ScreenSize.y = 600;
			}

			if (pNode->Attribute("RunFullSpeed"))
			{
				attribute = pNode->Attribute("RunFullSpeed");
				m_RunFullSpeed = (attribute == "yes") ? true : false;
			}
		}

		pNode = pRoot->FirstChildElement("Sound");
		if (pNode)
		{
			m_MusicVolume = atoi(pNode->Attribute("MusicVolume")) / 100.0f;
			m_SoundEffectsVolume = atoi(pNode->Attribute("SFXVolume")) / 100.0f;
		}

		pNode = pRoot->FirstChildElement("Multiplayer");
		if (pNode)
		{
			m_ExpectedPlayers = atoi(pNode->Attribute("ExpectedPlayers"));
			m_NumAIs = atoi(pNode->Attribute("NumAIs"));
			m_MaxPlayers = atoi(pNode->Attribute("MaxPlayers"));
			m_MaxAIs = atoi(pNode->Attribute("MaxAIs"));
			m_ListenPort = atoi(pNode->Attribute("ListenPort"));
			m_GameHost = pNode->Attribute("GameHost");
		}

		pNode = pRoot->FirstChildElement("ResCache");
		if (pNode)
		{
			string attribute = pNode->Attribute("UseDevelopmentDirectories");
			m_UseDevelopmentDirectories = (attribute == "yes") ? true : false;
		}
	}
}


bool IsOnlyInstance(LPCTSTR gameTitle)
{
	//Only one window at a time
	HANDLE handle = CreateMutex(NULL, TRUE, gameTitle);
	if (GetLastError() != ERROR_SUCCESS)
	{
		HWND hWnd = FindWindow(gameTitle, NULL);
		if (hWnd)
		{
			//Return to the game window if it already exsists
			ShowWindow(hWnd, SW_SHOWNORMAL);
			SetFocus(hWnd);
			SetForegroundWindow(hWnd);
			SetActiveWindow(hWnd);
			return false;
		}
	}
	return true;
}


const TCHAR* GetSaveGameDirectories(HWND hWnd, const TCHAR* gameAppDirectory)
{
	HRESULT hr;
	static TCHAR saveGameDirectory[MAX_PATH];
	TCHAR userDataPath[MAX_PATH];

	hr = SHGetSpecialFolderPath(hWnd, userDataPath, CSIDL_APPDATA, true);
	_tcscpy_s(saveGameDirectory, userDataPath);
	_tcscat_s(saveGameDirectory, _T("\\"));
	_tcscat_s(saveGameDirectory, gameAppDirectory);

	//check for directory
	if (0xffffffff == GetFileAttributes(saveGameDirectory))
	{
		if (SHCreateDirectoryEx(hWnd, saveGameDirectory, NULL) != ERROR_SUCCESS)
			return false;
	}

	_tcscat_s(saveGameDirectory, _T("\\"));

	return saveGameDirectory;
}


bool CheckForJoystick(HWND hWnd)
{
	JOYINFO joyinfo;
	UINT wNumDevs;
	BOOL bDev1Attached, bDev2Attached;

	if ((wNumDevs = joyGetNumDevs()) == 0)
		return false;
	bDev1Attached = joyGetPos(JOYSTICKID1, &joyinfo) != JOYERR_UNPLUGGED;
	bDev2Attached = joyGetPos(JOYSTICKID2, &joyinfo) != JOYERR_UNPLUGGED;
	if (bDev1Attached)
		joySetCapture(hWnd, JOYSTICKID1, 1000 / 30, true);
	if (bDev2Attached)
		joySetCapture(hWnd, JOYSTICKID2, 1000 / 30, true);

	return true;
}



