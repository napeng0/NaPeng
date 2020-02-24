#pragma once
#include"Graphic3D\Scene.h"


const DWORD g_QuitNoPrompt = MAKELPARAM(-1, -1);
const UINT g_MsgEndModal = (WM_USER + 100);


enum
{
	// Lower numbers get drawn first
	ZORDER_BACKGROUND,
	ZORDER_LAYER1,
	ZORDER_LAYER2,
	ZORDER_LAYER3,
	ZORDER_TOOLTIP
};


enum MessageBox_Questions {
	QUESTION_WHERES_THE_CD,
	QUESTION_QUIT_GAME,
};

class FontHandler;
class BaseUI;



class BaseUI : public IScreenElement
{
protected:
	int					m_PosX, m_PosY;
	int					m_Width, m_Height;
	optional<int>		m_Result;
	bool				m_bIsVisible;
public:
	BaseUI() { m_bIsVisible = true; m_PosX = m_PosY = 0; m_Width = 100; m_Height = 100; }
	virtual void VOnUpdate(int) { };
	virtual HRESULT VOnLostDevice() { return S_OK; }
	virtual bool VIsVisible() const { return m_bIsVisible; }
	virtual void VSetVisible(bool visible) { m_bIsVisible = visible; }
};



class ScreenElementScene : public IScreenElement, public Scene
{
public:
	ScreenElementScene(shared_ptr<IRenderer> renderer) : Scene(renderer) { }
	virtual ~ScreenElementScene(void)
	{
		WARNING("~ScreenElementScene()");
	}

	// IScreenElement Implementation
	virtual void VOnUpdate(int deltaMS) { OnUpdate(deltaMS); };
	virtual HRESULT VOnRestore()
	{
		OnRestore(); return S_OK;
	}
	virtual HRESULT VOnRender(double fTime, float fElapsedTime)
	{
		OnRender(); return S_OK;
	}
	virtual HRESULT VOnLostDevice()
	{
		OnLostDevice(); return S_OK;
	}
	virtual int VGetZOrder() const { return 0; }
	virtual void VSetZOrder(int const zOrder) { }

	virtual LRESULT CALLBACK VOnMsgProc(AppMsg msg) { return 0; }

	virtual bool VIsVisible() const { return true; }
	virtual void VSetVisible(bool visible) { }
	virtual bool VAddChild(ActorId id, shared_ptr<ISceneNode> kid) { return Scene::AddChild(id, kid); }
};