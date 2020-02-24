#pragma once
#include"GameCode\interface.h"
#include"GameCodeStd.h"
#include"GameCodeStd.h"


class ScreenElementScene;
class CameraNode;
class HumanView : public IGameView
{
	friend class GameCodeApp;

public:
	class Console
	{
		bool m_IsActive;

		std::queue<std::string> m_DisplayStrings;

		RECT m_ConsoleOutputRect;	
		RECT m_ConsoleInputRect;	

		Color m_InputColor;
		Color m_OutputColor;

		std::string m_CurrentOutputString;	
		std::string m_CurrentInputString;	

		int m_ConsoleInputSize;	//Height of the input console window

		int m_CursorBlinkTimer;	
		bool m_IsCursorOn;	

		bool m_IsShiftDown;	
		bool m_IsCapsLockDown;	

		bool m_IsExecuteStringOnUpdate;// Has a command to excute in next tick
	public:
		Console(void);

		~Console(void);

		void AddDisplayText(const std::string& newText);
		void SetDisplayText(const std::string& newText);

		void SetActive(const bool isActive) { m_IsActive = isActive; }
		bool IsActive(void) const { return m_IsActive; }

		void HandleKeyboardInput(const unsigned int keyVal, const unsigned int oemKeyVal, const bool isKeyDown);

		void Update(const int deltaMs);

		void Render();

		

		
	};

protected:
	GameViewId m_ViewId;
	ActorId m_ActorId;

	ProcessManager* m_pProcessManager;				

	DWORD m_CurrTick;		// time right now
	DWORD m_LastDraw;		// last time the game rendered
	bool m_RunFullSpeed;	

	BaseGameState m_GameState;					

	virtual void VRenderText() { };

	Console m_Console;

public:
	ScreenElementList m_ScreenElements;

	int m_PointerRadius;

public:

	HumanView(shared_ptr<IRenderer> renderer);
	virtual ~HumanView();

	bool LoadGame(TiXmlElement* pLevelData);
	virtual HRESULT VOnRestore();
	virtual HRESULT VOnLostDevice();
	virtual void VOnRender(double time, float elapsedTime);
	virtual GameViewType VGetType() { return GV_HUMAN; }
	virtual GameViewId VGetId() const { return m_ViewId; }

	virtual void VOnAttach(GameViewId vid, ActorId aid)
	{
		m_ViewId = vid;
		m_ActorId = aid;
	}
	virtual LRESULT CALLBACK VOnMsgProc(AppMsg msg);
	virtual void VOnUpdate(const int deltaMs);

	// Virtual methods to control the layering of interface elements
	virtual void VPushElement(shared_ptr<IScreenElement> pElement);
	virtual void VRemoveElement(shared_ptr<IScreenElement> pElement);

	void TogglePause(bool active);					

	shared_ptr<IPointerHandler> m_PointerHandler;
	shared_ptr<IKeyboardHandler> m_KeyboardHandler;

	// Audio
	bool InitAudio();
	ProcessManager* GetProcessManager() { return m_pProcessManager; }

	//Camera adjustments.
	virtual void VSetCameraOffset(const Vec4 & camOffset);

	shared_ptr<ScreenElementScene> m_pScene;
	shared_ptr<CameraNode> m_pCamera;

	virtual void VSetControlledActor(ActorId actorId) { m_ActorId = actorId; }

	// Event delegates
	void PlaySoundDelegate(IEventDataPtr pEventData);
	void GameStateDelegate(IEventDataPtr pEventData);

	Console& GetConsole(void)
	{
		return m_Console;
	}

private:
	void RegisterAllDelegates(void);
	void RemoveAllDelegates(void);

	protected:
		virtual bool VLoadGameDelegate(TiXmlElement* pLevelData) { VPushElement(m_pScene);  return true; }


};