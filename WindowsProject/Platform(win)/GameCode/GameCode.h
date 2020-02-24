#pragma once
#include"GameCode\interface.h"
#include"MainLoop\Initialization.h"
#include"Graphic3D\SceneNode.h"
#include"UI\UserInterface.h"
#include"GameCode\BaseGameLogic.h"
#include"GameCodeStd.h"

extern const UINT g_MsgEndModal;
extern const DWORD g_QuitNoPrompt;

class FontHandler;
class BaseUI;
class EventManager;
class LuaStateManager;
class BaseSocketManager;
class NetworkEventForwarder;
class HumanView;

class GameCodeApp
{
protected:
	HINSTANCE m_hInstance;
	bool m_WindowedMode;
	bool m_IsRunning;
	bool m_IsQuitRequested;
	bool m_IsQuitting;
	Rect m_rcDesktop;
	Point m_ScreenSize;
	int m_ColorDepth;
	bool m_IsEditorRunning;

	std::map<std::wstring, std::wstring> m_TextResource;
	std::map<std::wstring, UINT> m_Hotkeys;

	int m_HasModalDialog;

public:

	enum Renderer
	{
		RENDERER_UNKNOWN,
		RENDERER_D3D9,
		RENDERER_D3D11
	};

	shared_ptr<IRenderer> m_Renderer;
	
	BaseGameLogic *m_pGame= New BaseGameLogic;
	struct GameOptions m_Options;

	class ResCache* m_ResCache;
	TCHAR m_SaveGameDirectory[MAX_PATH];

	EventManager* m_pEventManager;

	class BaseSocketManager* m_pBaseSocketManager;
	class NetworkEventForwarder* m_pNetworkEventForwarder;
	
public:
	GameCodeApp();

	virtual TCHAR* VGetGameTitle() {};
	virtual TCHAR* VGetGameAppDirectory() {};
	virtual HICON VGetIcon() {};

	const Point &GetScreenSize() { return m_ScreenSize; }
	HWND GetHwnd();
	HINSTANCE GetInstance() { return m_hInstance; }
	virtual bool InitInstance(HINSTANCE hInstance, LPWSTR lpCmdLine, HWND hWnd = NULL, int screenWidth = SCREEN_WIDTH, int screenHeight = SCREEN_HEIGHT);

	static LRESULT CALLBACK MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pNoFutherProcessing, void* pUserContext);
	bool HasModalDialog() { return m_HasModalDialog; }
	void ForceModalExit() { PostMessage(GetHwnd(), g_MsgEndModal, 0, g_QuitNoPrompt); }

	LRESULT OnDisplayChange(int colorDepth, int width, int height);
	LRESULT OnPowerBroadcast(int event);
	LRESULT OnSysCommand(WPARAM wParam, LPARAM lParam);
	LRESULT OnClose();

	LRESULT OnAltEnter();
	LRESULT OnNcCreate(LPCREATESTRUCT cs) 
	{
		return true;
	}

	bool LoadStrings(std::string language);
	std::wstring GetString(std::wstring sId);
	UINT MapCharToKeycode(const char Hotkey);
	int Modal(shared_ptr<IScreenElement> pModalScreen, int defaultAnswer);

	//D3D stuff

	static Renderer GetRendererImpl();

	static bool CALLBACK IsD3D9DeviceAcceptable(D3DCAPS9* pCaps, D3DFORMAT adapterFormat, D3DFORMAT backBufferFormat, bool windowed, void* pUserContext);
	static HRESULT CALLBACK OnD3D9CreateDevice(IDirect3DDevice9* pd3dDevice, const D3DSURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext);
	static HRESULT CALLBACK OnD3D9ResetDevice(IDirect3DDevice9* pd3dDevice, const D3DSURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext);
	static void CALLBACK OnD3D9FrameRender(IDirect3DDevice9* pd3dDevice, double time, float elapsdTime, void* pUserContext);
	static void CALLBACK OnD3D9LostDevice(void* pUserContext);
	static void CALLBACK OnD3D9DestroyDevice(void* pUserContext);

	static bool CALLBACK IsD3D11DeviceAcceptable(const CD3D11EnumAdapterInfo* adapterInfo, UINT Output, const CD3D11EnumDeviceInfo* deviceInfo, DXGI_FORMAT backBufferFormat, bool windowded, void* pUserContext);
	static HRESULT CALLBACK OnD3D11CreateDevice(ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext);
	static HRESULT CALLBACK OnD3D11ResizedSwapChain(ID3D11Device* pd3dDevice, IDXGISwapChain* pSwapChain, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext);
	static void CALLBACK OnD3D11ReleasingSwapChain(void* pUserContext);
	static void CALLBACK OnD3D11DestroyDevice(void* pUserContex);
	static void CALLBACK OnD3D11FrameRender(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dContext, double time, float elapsedTime, void* pUserContext);

	static bool CALLBACK ModifyDeviceSettings(DXUTDeviceSettings* pDeviceSettings, void* pUserContext);
	static void CALLBACK OnUpdateGame(double time, float elapsedTime, void* pUserContext);

	//Game Code stuff
	virtual BaseGameLogic* VCreateGameAndView() {};
	virtual bool VLoadGame();
	class HumanView* GetHumanView();

	bool IsEditorRunning() { return m_IsEditorRunning; }

	//Socket manager
	bool AttachAsClient();

	//Main loop
	void AbortGame() { m_IsQuitting = true; }
	int GetExitCode() { return DXUTGetExitCode(); }
	bool IsRunning() { return m_IsRunning; }
	void SetQuitting(bool quitting) { m_IsQuitting = quitting; }

	BaseGameLogic* GetGameLogic() const { return m_pGame; }

protected:
	int PumpUntilMessage(UINT msgEnd, WPARAM* pWparam, LPARAM* pLParam);
	int EatSpecificMessages(UINT msgType, optional<LPARAM> lParam, optional<WPARAM> wParam);
	void FlashWhileMinimized();

	virtual void VCreateNetworkEventForwarder();
	virtual void VDestroyNetworkEventForwarder();
	virtual void VRegisterGameEvents() {}

private:
	void RegisterEngineEvents();
};

extern GameCodeApp* g_pApp;