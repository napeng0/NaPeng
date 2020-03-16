#include"GameCodeStd.h"

#include"MainLoop\Initialization.h"
#include"GameCode\BaseGameLogic.h"
#include"Graphic3D\Renderer.h"
#include"EventManager\EventManager.h"
#include"NetWork\NetWork.h"
#include"LUA\LuaStateManager.h"
#include"LUA\ScriptExport.h"
#include"LUA\ScriptProcess.h"
#include"ResourceCache\ResCache.h"
#include"ResourceCache\XmlResource.h"
#include"UI\UserInterface.h"
#include"UI\MessageBox.h"
#include"UI\HumanView.h"
#include"Utilities\Math.h"
#include"Utilities\String.h"
#include"Actors\BaseScriptComponent.h"
#include"Physics\PhysicsEventListener.h"
#include"EventManager\Events.h"
#include"Audio\Audio.h"
#include"GameCode\BaseGameLogic.h"

#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "dxut.lib")
#pragma comment(lib, "dxutopt.lib")

#define MAX_LOADSTRING 100


#if defined(_M_IX86)
#if defined(_DEBUG)
#pragma comment(lib, "bulletcollision_debug.lib")
#pragma comment(lib, "bulletdynamics_debug.lib")
#pragma comment(lib, "linearmath_debug.lib")
#else
#pragma comment(lib, "bulletcollision.lib")
#pragma comment(lib, "bulletdynamics.lib")
#pragma comment(lib, "linearmath.lib")
#endif
#elif defined(_M_X64)
#if defined(_DEBUG)
#pragma comment(lib, "bulletcollision_x64__debug.lib")
#pragma comment(lib, "bulletdynamics_x64__debug.lib")
#pragma comment(lib, "linearmath_x64_debug.lib")
#else
#pragma comment(lib, "bulletcollision_x64.lib")
#pragma comment(lib, "bulletdynamics_x64.lib")
#pragma comment(lib, "linearmath_x64.lib")
#endif
#endif



const char* SCRIPT_PREINIT_FILE = "Scripts\\PreInit.lua";



GameCodeApp* g_pApp = NULL;


GameCodeApp::GameCodeApp()
{
	g_pApp = this;
	m_pGame = NULL;

	m_rcDesktop.bottom = m_rcDesktop.left = m_rcDesktop.right = m_rcDesktop.top = 0;
	m_ScreenSize = Point(0, 0);
	m_ColorDepth = 32;

	m_IsRunning = false;
	m_IsEditorRunning = false;

	m_pEventManager = NULL;
	m_ResCache = NULL;

	m_pNetworkEventForwarder = NULL;
	m_pBaseSocketManager = NULL;

	m_IsQuitRequested = false;
	m_IsQuitting = false;
	m_HasModalDialog = 0;
}


HWND GameCodeApp::GetHwnd()
{
	return DXUTGetHWND();
}




bool GameCodeApp::InitInstance(HINSTANCE hInstance, LPWSTR lpCmdLine, HWND hWnd, int screenWidth, int screenHeight)
{

	// We don't need a mouse cursor by default
	SetCursor(NULL);

	// Check for adequate machine resources.
	bool resourceCheck = false;
	while (!resourceCheck)
	{
		const DWORDLONG physicalRAM = 512 * MEGABYTE;
		const DWORDLONG virtualRAM = 1024 * MEGABYTE;
		const DWORDLONG diskSpace = 10 * MEGABYTE;
		if (!CheckStorage(diskSpace))
			return false;

		const DWORD minCpuSpeed = 1300;			// 1.3Ghz
		DWORD thisCPU = ReadCPUSpeed();
		if (thisCPU < minCpuSpeed)
		{
			ERROR("GetCPUSpeed reports CPU is too slow for this game.");
			return false;
		}

		resourceCheck = true;
	}

	m_hInstance = hInstance;

	// Register all events
	RegisterEngineEvents();
	VRegisterGameEvents();

	IResourceFile* zipFile = (m_IsEditorRunning || m_Options.m_UseDevelopmentDirectories) ?
		New DevelopmentResourceZipFile(L"Assets.zip", DevelopmentResourceZipFile::Editor) :
		New ResourceZipFile(L"Assets.zip");

	m_ResCache = New ResCache(50, zipFile);

	if (!m_ResCache->Init())
	{
		ERROR("Failed to initialize resource cache!  Are your paths set up correctly?");
		return false;
	}

	extern shared_ptr<IResourceLoader> CreateWAVResourceLoader();
	extern shared_ptr<IResourceLoader> CreateOGGResourceLoader();
	extern shared_ptr<IResourceLoader> CreateDDSResourceLoader();
	extern shared_ptr<IResourceLoader> CreateJPGResourceLoader();
	extern shared_ptr<IResourceLoader> CreateXmlResourceLoader();
	extern shared_ptr<IResourceLoader> CreateSdkMeshResourceLoader();
	extern shared_ptr<IResourceLoader> CreateScriptResourceLoader();

	
	m_ResCache->RegisterLoader(CreateWAVResourceLoader());
	m_ResCache->RegisterLoader(CreateOGGResourceLoader());
	m_ResCache->RegisterLoader(CreateDDSResourceLoader());
	m_ResCache->RegisterLoader(CreateJPGResourceLoader());
	m_ResCache->RegisterLoader(CreateXmlResourceLoader());
	m_ResCache->RegisterLoader(CreateSdkMeshResourceLoader());
	m_ResCache->RegisterLoader(CreateScriptResourceLoader());

	if (!LoadStrings("English"))
	{
		ERROR("Failed to load strings");
		return false;
	}

	if (!LuaStateManager::Create())
	{
		ERROR("Failed to initialize Lua");
		return false;
	}

	// Load the preinit file. 
	{
		Resource resource(SCRIPT_PREINIT_FILE);
		shared_ptr<ResHandle> pResourceHandle = m_ResCache->GetHandle(&resource);  
	}

	// Register function exported from C++
	ScriptExports::Register();
	ScriptProcess::RegisterScriptClass();
	BaseScriptComponent::RegisterScriptFunctions();

	// The event manager should be created next so that subsystems can hook in as desired.
	// Discussed in Chapter 5, page 144
	m_pEventManager = New EventManager("GameCodeApp Event Mgr", true);
	if (!m_pEventManager)
	{
		ERROR("Failed to create EventManager.");
		return false;
	}

	// DXUTInit, DXUTCreateWindow 
	DXUTInit(true, true, lpCmdLine, true); 

	if (hWnd == NULL)
	{
		DXUTCreateWindow(VGetGameTitle(), hInstance, VGetIcon());
	}
	else
	{
		DXUTSetWindow(hWnd, hWnd, hWnd);
	}

	if (!GetHwnd())
	{
		return FALSE;
	}
	SetWindowText(GetHwnd(), VGetGameTitle());

	// initialize the directory location you can store save game files
	_tcscpy_s(m_SaveGameDirectory, GetSaveGameDirectory(GetHwnd(), VGetGameAppDirectory()));

	// DXUTCreateDevice 
	m_ScreenSize = Point(screenWidth, screenHeight);

	
	//DXUTCreateDevice( D3D_FEATURE_LEVEL_9_3, true, screenWidth, screenHeight);
	DXUTCreateDevice(D3D_FEATURE_LEVEL_10_1, true, screenWidth, screenHeight);

	if (GetRendererImpl() == RENDERER_D3D9)
	{
		m_Renderer = shared_ptr<IRenderer>(New D3DRenderer9());
	}
	else if (GetRendererImpl() == RENDERER_D3D11)
	{
		m_Renderer = shared_ptr<IRenderer>(New D3DRenderer11());
	}
	m_Renderer->VSetBackgroundColor(255, 20, 20, 200);
	m_Renderer->VOnRestore();


	
	m_pGame = VCreateGameAndView();
	if (!m_pGame)
		return false;

	// Now that all the major systems are initialized, preload resources 
	
	m_ResCache->Preload("*.ogg", NULL);
	m_ResCache->Preload("*.dds", NULL);
	m_ResCache->Preload("*.jpg", NULL);

	if (GameCodeApp::GetRendererImpl() == GameCodeApp::RENDERER_D3D11)
		m_ResCache->Preload("*.sdkmesh", NULL);

	CheckForJoystick(GetHwnd());

	m_IsRunning = true;

	return TRUE;
}

bool GameCodeApp::VLoadGame(void)
{
	// Read the game options 
	return m_pGame->VLoadGame(m_Options.m_Level.c_str());
}

void GameCodeApp::RegisterEngineEvents(void)
{
	REGISTER_EVENT(EvtData_Environment_Loaded);
	REGISTER_EVENT(EvtData_New_Actor);
	REGISTER_EVENT(EvtData_Move_Actor);
	REGISTER_EVENT(EvtData_Destroy_Actor);
	REGISTER_EVENT(EvtData_Request_New_Actor);
	REGISTER_EVENT(EvtData_Network_Player_Actor_Assignment);
}


bool GameCodeApp::LoadStrings(std::string language)
{
	std::string languageFile = "Strings\\";
	languageFile += language;
	languageFile += ".xml";

	TiXmlElement* pRoot = XmlResourceLoader::LoadAndReturnRootXmlElement(languageFile.c_str());
	if (!pRoot)
	{
		ERROR("Strings are missing.");
		return false;
	}

	// Loop through each child element and load the component
	for (TiXmlElement* pElem = pRoot->FirstChildElement(); pElem; pElem = pElem->NextSiblingElement())
	{
		const char *pKey = pElem->Attribute("id");
		const char *pText = pElem->Attribute("value");
		const char *pHotkey = pElem->Attribute("hotkey");
		if (pKey && pText)
		{
			wchar_t wideKey[64];
			wchar_t wideText[1024];
			AnsiToWideCch(wideKey, pKey, 64);
			AnsiToWideCch(wideText, pText, 1024);
			m_TextResource[std::wstring(wideKey)] = std::wstring(wideText);

			if (pHotkey)
			{
				m_Hotkeys[std::wstring(wideKey)] = MapCharToKeycode(*pHotkey);
			}
		}
	}
	return true;
}

UINT GameCodeApp::MapCharToKeycode(const char pHotKey)
{
	if (pHotKey >= '0' && pHotKey <= '9')
		return 0x30 + pHotKey - '0';

	if (pHotKey >= 'A' && pHotKey <= 'Z')
		return 0x41 + pHotKey - 'A';

	ASSERT(0 && "Platform specific hotkey is not defined");
	return 0;
}



std::wstring GameCodeApp::GetString(std::wstring sID)
{
	auto localizedString = m_TextResource.find(sID);
	if (localizedString == m_TextResource.end())
	{
		ASSERT(0 && "String not found!");
		return L"";
	}
	return localizedString->second;
}



LRESULT CALLBACK GameCodeApp::MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing, void* pUserContext)
{
	// Always allow dialog resource manager calls to handle global messages
	// so GUI state is updated correctly
	*pbNoFurtherProcessing = D3DRenderer::g_DialogResourceManager.MsgProc(hWnd, uMsg, wParam, lParam);
	if (*pbNoFurtherProcessing)
		return 0;

	LRESULT result = 0;

	switch (uMsg)
	{
		case WM_POWERBROADCAST:
		{
			int event = (int)wParam;
			result = g_pApp->OnPowerBroadcast(event);
			break;
		}

		case WM_DISPLAYCHANGE:
		{
			int colorDepth = (int)wParam;
			int width = (int)(short)LOWORD(lParam);
			int height = (int)(short)HIWORD(lParam);

			result = g_pApp->OnDisplayChange(colorDepth, width, height);
			break;
		}

		case WM_SYSCOMMAND:
		{
			result = g_pApp->OnSysCommand(wParam, lParam);
			if (result)
			{
				*pbNoFurtherProcessing = true;
			}
			break;
		}

		case WM_SYSKEYDOWN:
		{
			if (wParam == VK_RETURN)
			{
				*pbNoFurtherProcessing = true;
				return g_pApp->OnAltEnter();
			}
			return DefWindowProc(hWnd, uMsg, wParam, lParam);
		}


		case WM_CLOSE:
		{

			if (g_pApp->m_IsQuitting)
			{
				result = g_pApp->OnClose();
			}
			else
			{
				*pbNoFurtherProcessing = true;
			}
			break;
		}


		case WM_KEYDOWN:
		case WM_KEYUP:
		case WM_CHAR:
		case WM_MOUSEMOVE:
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
		case MM_JOY1BUTTONDOWN:
		case MM_JOY1BUTTONUP:
		case MM_JOY1MOVE:
		case MM_JOY1ZMOVE:
		case MM_JOY2BUTTONDOWN:
		case MM_JOY2BUTTONUP:
		case MM_JOY2MOVE:
		case MM_JOY2ZMOVE:
		{

			if (g_pApp->m_pGame)
			{
				BaseGameLogic *pGame = g_pApp->m_pGame;
				AppMsg msg;
				msg.m_hWnd = hWnd;
				msg.m_uMsg = uMsg;
				msg.m_wParam = wParam;
				msg.m_lParam = lParam;
				for (GameViewList::reverse_iterator i = pGame->m_GameViews.rbegin(); i != pGame->m_GameViews.rend(); ++i)
				{
					if ((*i)->VOnMsgProc(msg))
					{
						result = true;
						break;
					}
				}
			}
			break;
		}


	}

	return result;


	
}







LRESULT GameCodeApp::OnDisplayChange(int colorDepth, int width, int height)
{
	m_rcDesktop.left = 0;
	m_rcDesktop.top = 0;
	m_rcDesktop.right = width;
	m_rcDesktop.bottom = height;
	m_ColorDepth = colorDepth;
	return 0;
}



LRESULT GameCodeApp::OnPowerBroadcast(int event)
{
	// Don't allow the game to go into sleep mode
	if (event == PBT_APMQUERYSUSPEND)
		return BROADCAST_QUERY_DENY;
	else if (event == PBT_APMBATTERYLOW)
	{
		AbortGame();
	}

	return true;
}



LRESULT GameCodeApp::OnSysCommand(WPARAM wParam, LPARAM lParam)
{
	switch (wParam)
	{
	case SC_MAXIMIZE:
	{
		// If windowed and ready...
		if (m_WindowedMode && IsRunning())
		{
			// Make maximize into FULLSCREEN toggle
			OnAltEnter();
		}
	}
	return 0;

	case SC_CLOSE:
	{


		// If closing, prompt to close if this isn't a forced quit
		if (lParam != g_QuitNoPrompt)
		{
	
			if (m_IsQuitRequested)
				return true;


			// Quit requested
			m_IsQuitRequested = true;
			// Prompt
			if (MessageBox::Ask(QUESTION_QUIT_GAME) == IDNO)
			{
				m_IsQuitRequested = false;

				return true;
			}
		}

		m_IsQuitting = true;

		// Is there a game modal dialog up?
		if (HasModalDialog())
		{
			// Close the modal
			// and keep posting close to the app
			ForceModalExit();

			// Reissue the close to the app
			PostMessage(GetHwnd(), WM_SYSCOMMAND, SC_CLOSE, g_QuitNoPrompt);

			m_IsQuitRequested = false;

			return true;
		}

		// Reset the quit after any other dialogs have popped up from this close
		m_IsQuitRequested = false;
	}
	return 0;

	default:
		return DefWindowProc(GetHwnd(), WM_SYSCOMMAND, wParam, lParam);
	}

	return 0;
}



LRESULT GameCodeApp::OnClose()
{
	

	SAFE_DELETE(m_pGame);

	DestroyWindow(GetHwnd());

	VDestroyNetworkEventForwarder();

	SAFE_DELETE(m_pBaseSocketManager);

	SAFE_DELETE(m_pEventManager);

	BaseScriptComponent::UnregisterScriptFunctions();
	ScriptExports::Unregister();
	LuaStateManager::Destroy();

	SAFE_DELETE(m_ResCache);

	return 0;
}





void GameCodeApp::FlashWhileMinimized()
{
	// Flash the application on the taskbar
	// until it's restored.
	if (!GetHwnd())
		return;

	// Blink the application if we are minimized,
	// waiting until we are no longer minimized
	if (IsIconic(GetHwnd()))
	{
		// Make sure the app is up when creating a new screen
		DWORD now = timeGetTime();
		DWORD then = now;
		MSG msg;

		FlashWindow(GetHwnd(), true);

		while (true)
		{
			if (PeekMessage(&msg, NULL, 0, 0, 0))
			{
				if (msg.message != WM_SYSCOMMAND || msg.wParam != SC_CLOSE)
				{
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
				if (!IsIconic(GetHwnd()))
				{
					FlashWindow(GetHwnd(), false);
					break;
				}
			}
			else
			{
				now = timeGetTime();
				DWORD timeSpan = now > then ? (now - then) : (then - now);
				if (timeSpan > 1000)
				{
					then = now;
					FlashWindow(GetHwnd(), true);
				}
			}
		}
	}
}





LRESULT GameCodeApp::OnAltEnter()
{
	DXUTToggleFullScreen();
	return 0;
}


HumanView* GameCodeApp::GetHumanView()
{
	HumanView *pView = NULL;
	for (GameViewList::iterator i = m_pGame->m_GameViews.begin(); i != m_pGame->m_GameViews.end(); ++i)
	{
		if ((*i)->VGetType() == GV_HUMAN)
		{
			shared_ptr<IGameView> pIGameView(*i);
			pView = (HumanView*)(&*pIGameView);
			break;
		}
	}
	return pView;
}


int GameCodeApp::Modal(shared_ptr<IScreenElement> pModalScreen, int defaultAnswer)
{

	HumanView* pView = GetHumanView();

	if (!pView)
	{
		return defaultAnswer;
	}

	if (m_HasModalDialog & 0x10000000)
	{
		ASSERT(0 && "Too Many nested dialogs!");
		return defaultAnswer;
	}

	ASSERT(GetHwnd() != NULL && _T("Main Window is NULL!"));
	if ((GetHwnd() != NULL) && IsIconic(GetHwnd()))
	{
		FlashWhileMinimized();
	}

	m_HasModalDialog <<= 1;
	m_HasModalDialog |= 1;

	pView->VPushElement(pModalScreen);

	LPARAM lParam = 0;
	int result = PumpUntilMessage(g_MsgEndModal, NULL, &lParam);

	if (lParam != 0)
	{
		if (lParam == g_QuitNoPrompt)
			result = defaultAnswer;
		else
			result = (int)lParam;
	}

	pView->VRemoveElement(pModalScreen);
	m_HasModalDialog >>= 1;

	return result;
}


int GameCodeApp::PumpUntilMessage(UINT msgEnd, WPARAM* pWParam, LPARAM* pLParam)
{
	int currentTime = timeGetTime();
	MSG msg;
	for (;; )
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
		{
			if (msg.message == WM_CLOSE)
			{
				m_IsQuitting = true;
				GetMessage(&msg, NULL, 0, 0);
				break;
			}
			else
			{
				// Default processing
				if (GetMessage(&msg, NULL, NULL, NULL))
				{
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}

				if (msg.message == msgEnd)
					break;
			}
		}
		else
		{
			// Update the game views

			if (m_pGame)
			{
				int timeNow = timeGetTime();
				int deltaMilliseconds = timeNow - currentTime;
				for (GameViewList::iterator i = m_pGame->m_GameViews.begin(); i != m_pGame->m_GameViews.end(); ++i)
				{
					(*i)->VOnUpdate(deltaMilliseconds);
				}
				currentTime = timeNow;
				DXUTRender3DEnvironment();
			}
		}
	}
	if (pLParam)
		*pLParam = msg.lParam;
	if (pWParam)
		*pWParam = msg.wParam;

	return 0;
}



int	GameCodeApp::EatSpecificMessages(UINT msgType, optional<LPARAM> lParam, optional<WPARAM> wParam)
{
	bool done = false;

	while (!done)
	{
		MSG msg;

		if (PeekMessage(&msg, NULL, msgType, msgType, PM_NOREMOVE))
		{
			bool valid = true;

			if (lParam.valid())
			{
				valid &= (*lParam == msg.lParam);
			}

			if (wParam.valid())
			{
				valid &= (*wParam == msg.wParam);
			}

			if (valid)
			{
				GetMessage(&msg, NULL, msgType, msgType);
			}
			else
			{
				done = true;
			}
		}
		else
		{
			done = true;	
		}
	}

	return 0;
}




HRESULT CALLBACK GameCodeApp::OnD3D9ResetDevice(IDirect3DDevice9* pd3dDevice,
	const D3DSURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext)
{
	HRESULT hr;

	if (g_pApp->m_Renderer)
	{
		V_RETURN(g_pApp->m_Renderer->VOnRestore());
	}

	if (g_pApp->m_pGame)
	{
		BaseGameLogic *pGame = g_pApp->m_pGame;
		for (GameViewList::iterator i = pGame->m_GameViews.begin(); i != pGame->m_GameViews.end(); ++i)
		{
			V_RETURN((*i)->VOnRestore());
		}
	}

	return S_OK;
}



GameCodeApp::Renderer GameCodeApp::GetRendererImpl()
{
	if (DXUTGetDeviceSettings().ver == DXUT_D3D9_DEVICE)
		return RENDERER_D3D9;
	else
		return RENDERER_D3D11;
	return RENDERER_UNKNOWN;
};



bool CALLBACK GameCodeApp::IsD3D9DeviceAcceptable(D3DCAPS9* pCaps, D3DFORMAT AdapterFormat,
	D3DFORMAT BackBufferFormat, bool bWindowed, void* pUserContext)
{
	// Skip backbuffer formats that don't support alpha blending
	IDirect3D9* pD3D = DXUTGetD3D9Object();
	if (FAILED(pD3D->CheckDeviceFormat(pCaps->AdapterOrdinal, pCaps->DeviceType,
		AdapterFormat, D3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING,
		D3DRTYPE_TEXTURE, BackBufferFormat)))
		return false;

	// No fallback defined by this app, so reject any device that 
	// doesn't support at least ps2.0
	if (pCaps->PixelShaderVersion < D3DPS_VERSION(2, 0))
		return false;

	return true;
}




void CALLBACK GameCodeApp::OnD3D9LostDevice(void* pUserContext)
{
	D3DRenderer::g_DialogResourceManager.OnD3D9LostDevice();

	if (g_pApp->m_pGame)
	{
		BaseGameLogic *pGame = g_pApp->m_pGame;
		for (GameViewList::iterator i = pGame->m_GameViews.begin(); i != pGame->m_GameViews.end(); ++i)
		{
			(*i)->VOnLostDevice();
		}
	}
}



bool CALLBACK GameCodeApp::IsD3D11DeviceAcceptable(const CD3D11EnumAdapterInfo *AdapterInfo, UINT Output, const CD3D11EnumDeviceInfo *DeviceInfo,
	DXGI_FORMAT BackBufferFormat, bool bWindowed, void* pUserContext)
{
	return true;
}



HRESULT CALLBACK GameCodeApp::OnD3D11CreateDevice(ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc,
	void* pUserContext)
{
	HRESULT hr;

	ID3D11DeviceContext* pd3dImmediateContext = DXUTGetD3D11DeviceContext();
	V_RETURN(D3DRenderer::g_DialogResourceManager.OnD3D11CreateDevice(pd3dDevice, pd3dImmediateContext));

	return S_OK;
}



HRESULT CALLBACK GameCodeApp::OnD3D11ResizedSwapChain(ID3D11Device* pd3dDevice, IDXGISwapChain* pSwapChain,
	const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext)
{
	HRESULT hr;

	V_RETURN(D3DRenderer::g_DialogResourceManager.OnD3D11ResizedSwapChain(pd3dDevice, pBackBufferSurfaceDesc));

	if (g_pApp->m_pGame)
	{
		BaseGameLogic *pGame = g_pApp->m_pGame;
		for (GameViewList::iterator i = pGame->m_GameViews.begin(); i != pGame->m_GameViews.end(); ++i)
		{
			(*i)->VOnRestore();
		}
	}

	return S_OK;
}



void CALLBACK GameCodeApp::OnD3D11FrameRender(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, double fTime,
	float fElapsedTime, void* pUserContext)
{
	BaseGameLogic *pGame = g_pApp->m_pGame;

	for (GameViewList::iterator i = pGame->m_GameViews.begin(),
		end = pGame->m_GameViews.end(); i != end; ++i)
	{
		(*i)->VOnRender(fTime, fElapsedTime);
	}

	g_pApp->m_pGame->VRenderDiagnostics();
}



void CALLBACK GameCodeApp::OnD3D11ReleasingSwapChain(void* pUserContext)
{
	D3DRenderer::g_DialogResourceManager.OnD3D11ReleasingSwapChain();
}



void CALLBACK GameCodeApp::OnD3D11DestroyDevice(void* pUserContext)
{
	if (g_pApp->m_Renderer)  
		g_pApp->m_Renderer->VShutDown();
	D3DRenderer::g_DialogResourceManager.OnD3D11DestroyDevice();
	g_pApp->m_Renderer = shared_ptr<IRenderer>(NULL);
}



bool CALLBACK GameCodeApp::ModifyDeviceSettings(DXUTDeviceSettings* pDeviceSettings, void* pUserContext)
{
	if (pDeviceSettings->ver == DXUT_D3D9_DEVICE)
	{
		IDirect3D9* pD3D = DXUTGetD3D9Object();
		D3DCAPS9 Caps;
		pD3D->GetDeviceCaps(pDeviceSettings->d3d9.AdapterOrdinal, pDeviceSettings->d3d9.DeviceType, &Caps);

		
		if ((Caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT) == 0 ||
			Caps.VertexShaderVersion < D3DVS_VERSION(1, 1))
		{
			pDeviceSettings->d3d9.BehaviorFlags = D3DCREATE_SOFTWARE_VERTEXPROCESSING;
		}


#ifdef DEBUG_VS
		if (pDeviceSettings->d3d9.DeviceType != D3DDEVTYPE_REF)
		{
			pDeviceSettings->d3d9.BehaviorFlags &= ~D3DCREATE_HARDWARE_VERTEXPROCESSING;
			pDeviceSettings->d3d9.BehaviorFlags &= ~D3DCREATE_PUREDEVICE;
			pDeviceSettings->d3d9.BehaviorFlags |= D3DCREATE_SOFTWARE_VERTEXPROCESSING;
		}
#endif
#ifdef DEBUG_PS
		pDeviceSettings->d3d9.DeviceType = D3DDEVTYPE_REF;
#endif
	}

	
	static bool s_bFirstTime = true;
	if (s_bFirstTime)
	{
		s_bFirstTime = false;
		if ((DXUT_D3D9_DEVICE == pDeviceSettings->ver && pDeviceSettings->d3d9.DeviceType == D3DDEVTYPE_REF) ||
			(DXUT_D3D11_DEVICE == pDeviceSettings->ver &&
				pDeviceSettings->d3d11.DriverType == D3D_DRIVER_TYPE_REFERENCE))
		{
			DXUTDisplaySwitchingToREFWarning(pDeviceSettings->ver);
		}

	}

	return true;
}






void CALLBACK GameCodeApp::OnUpdateGame(double fTime, float fElapsedTime, void* pUserContext)
{
	if (g_pApp->HasModalDialog())
	{
		// Don't update the game if a modal dialog is up.
		return;
	}

	if (g_pApp->m_IsQuitting)
	{
		PostMessage(g_pApp->GetHwnd(), WM_CLOSE, 0, 0);
	}

	if (g_pApp->m_pGame)
	{
		IEventManager::Get()->VUpdate(20); // Allow event queue to process for up to 20 ms

		if (g_pApp->m_pBaseSocketManager)
			g_pApp->m_pBaseSocketManager->DoSelect(0);	// Pause 0 microseconds

		g_pApp->m_pGame->VOnUpdate(float(fTime), fElapsedTime);
	}
}







void CALLBACK GameCodeApp::OnD3D9FrameRender(IDirect3DDevice9* pd3dDevice, double fTime, float fElapsedTime, void* pUserContext)
{
	BaseGameLogic *pGame = g_pApp->m_pGame;

	for (GameViewList::iterator i = pGame->m_GameViews.begin(),
		end = pGame->m_GameViews.end(); i != end; ++i)
	{
		(*i)->VOnRender(fTime, fElapsedTime);
	}

	g_pApp->m_pGame->VRenderDiagnostics();
}



HRESULT CALLBACK GameCodeApp::OnD3D9CreateDevice(IDirect3DDevice9* pd3dDevice, const D3DSURFACE_DESC* pBackBufferSurfaceDesc,
	void* pUserContext)
{
	HRESULT hr;

	V_RETURN(D3DRenderer::g_DialogResourceManager.OnD3D9CreateDevice(pd3dDevice));

	return S_OK;
}


void CALLBACK GameCodeApp::OnD3D9DestroyDevice(void* pUserContext)
{
	g_pApp->m_Renderer->VShutDown();
	D3DRenderer::g_DialogResourceManager.OnD3D9DestroyDevice();
	g_pApp->m_Renderer = shared_ptr<IRenderer>(NULL);
}


bool GameCodeApp::AttachAsClient()
{
	ClientSocketManager *pClient = New ClientSocketManager(g_pApp->m_Options.m_GameHost, g_pApp->m_Options.m_ListenPort);
	if (!pClient->Connect())
	{
		return false;
	}
	g_pApp->m_pBaseSocketManager = pClient;
	VCreateNetworkEventForwarder();

	return true;
}


// Any events that will be received from the server logic should be here!
void GameCodeApp::VCreateNetworkEventForwarder(void)
{
	if (m_pNetworkEventForwarder != NULL)
	{
		ERROR("Overwriting network event forwarder£¡");
		SAFE_DELETE(m_pNetworkEventForwarder);
	}

	m_pNetworkEventForwarder = New NetworkEventForwarder(0);

	IEventManager* pGlobalEventManager = IEventManager::Get();
	pGlobalEventManager->VAddListener(MakeDelegate(m_pNetworkEventForwarder, &NetworkEventForwarder::ForwardEvent), EvtData_Request_New_Actor::s_EventType);
	pGlobalEventManager->VAddListener(MakeDelegate(m_pNetworkEventForwarder, &NetworkEventForwarder::ForwardEvent), EvtData_Environment_Loaded::s_EventType);
	pGlobalEventManager->VAddListener(MakeDelegate(m_pNetworkEventForwarder, &NetworkEventForwarder::ForwardEvent), EvtData_PhysCollision::s_EventType);

}

void GameCodeApp::VDestroyNetworkEventForwarder(void)
{
	if (m_pNetworkEventForwarder)
	{
		IEventManager* pGlobalEventManager = IEventManager::Get();
		pGlobalEventManager->VRemoveListener(MakeDelegate(m_pNetworkEventForwarder, &NetworkEventForwarder::ForwardEvent), EvtData_Request_New_Actor::s_EventType);
		pGlobalEventManager->VRemoveListener(MakeDelegate(m_pNetworkEventForwarder, &NetworkEventForwarder::ForwardEvent), EvtData_Environment_Loaded::s_EventType);
		pGlobalEventManager->VRemoveListener(MakeDelegate(m_pNetworkEventForwarder, &NetworkEventForwarder::ForwardEvent), EvtData_PhysCollision::s_EventType);
		SAFE_DELETE(m_pNetworkEventForwarder);
	}
}


INT WINAPI GameCode(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPWSTR    lpCmdLine,
	int       nCmdShow)
{
	int tmpDbgFlag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);

	tmpDbgFlag |= _CRTDBG_LEAK_CHECK_DF;

	_CrtSetDbgFlag(tmpDbgFlag);

	
	Logger::Init("logging.xml");

	g_pApp->m_Options.Init("PlayerOptions.xml", lpCmdLine);


	DXUTSetCallbackMsgProc(GameCodeApp::MsgProc);
	DXUTSetCallbackFrameMove(GameCodeApp::OnUpdateGame);
	DXUTSetCallbackDeviceChanging(GameCodeApp::ModifyDeviceSettings);

	if (g_pApp->m_Options.m_Renderer == "Direct3D 9")
	{
		DXUTSetCallbackD3D9DeviceAcceptable(GameCodeApp::IsD3D9DeviceAcceptable);
		DXUTSetCallbackD3D9DeviceCreated(GameCodeApp::OnD3D9CreateDevice);
		DXUTSetCallbackD3D9DeviceReset(GameCodeApp::OnD3D9ResetDevice);
		DXUTSetCallbackD3D9DeviceLost(GameCodeApp::OnD3D9LostDevice);
		DXUTSetCallbackD3D9DeviceDestroyed(GameCodeApp::OnD3D9DestroyDevice);
		DXUTSetCallbackD3D9FrameRender(GameCodeApp::OnD3D9FrameRender);
	}
	else if (g_pApp->m_Options.m_Renderer == "Direct3D 11")
	{
		DXUTSetCallbackD3D11DeviceAcceptable(GameCodeApp::IsD3D11DeviceAcceptable);
		DXUTSetCallbackD3D11DeviceCreated(GameCodeApp::OnD3D11CreateDevice);
		DXUTSetCallbackD3D11SwapChainResized(GameCodeApp::OnD3D11ResizedSwapChain);
		DXUTSetCallbackD3D11SwapChainReleasing(GameCodeApp::OnD3D11ReleasingSwapChain);
		DXUTSetCallbackD3D11DeviceDestroyed(GameCodeApp::OnD3D11DestroyDevice);
		DXUTSetCallbackD3D11FrameRender(GameCodeApp::OnD3D11FrameRender);
	}
	else
	{
		ASSERT(0 && "Unknown renderer specified in game options.");
		return false;
	}

	// Show the cursor and clip it when in full screen
	DXUTSetCursorSettings(true, true);

	// Perform application initialization
	if (!g_pApp->InitInstance(hInstance, lpCmdLine, 0, g_pApp->m_Options.m_ScreenSize.x, g_pApp->m_Options.m_ScreenSize.y))
	{
		return FALSE;
	}


	DXUTMainLoop();
	DXUTShutdown();

	Logger::Destroy();

	return g_pApp->GetExitCode();
}



