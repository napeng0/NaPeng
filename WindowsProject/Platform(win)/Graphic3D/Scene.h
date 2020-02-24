#pragma once
#include"geometry.h"
#include"SceneNode.h"
#include"EventManager\Events.h"



class CameraNode;
class SkyNode;
class LightNode;
class LightManager;

typedef std::map<ActorId, shared_ptr<ISceneNode> > SceneActorMap;


class Scene
{
protected:
	shared_ptr<SceneNode> m_Root;
	shared_ptr<CameraNode> m_Camera;
	shared_ptr<IRenderer> m_Renderer;
	ID3DXMatrixStack *m_MatrixStack;
	AlphaSceneNodes m_AlphaSceneNodes;
	SceneActorMap m_ActorMap;
	LightManager			*m_LightManager;
	void RenderAlphaPass();


public:
	Scene(shared_ptr<IRenderer> renderer);
	virtual ~Scene();

	HRESULT OnRender();
	HRESULT OnRestore();
	HRESULT OnLostDevice();
	HRESULT OnUpdate(const int deltaMilliseconds);
	shared_ptr<ISceneNode> FindActor(ActorId id);
	bool AddChild(ActorId id, shared_ptr<ISceneNode> kid);
	bool RemoveChild(ActorId id);

	// Event delegates
	void NewRenderComponentDelegate(IEventDataPtr pEventData);
	void ModifiedRenderComponentDelegate(IEventDataPtr pEventData);			// added post-press!
	void DestroyActorDelegate(IEventDataPtr pEventData);
	void MoveActorDelegate(IEventDataPtr pEventData);

	void SetCamera(shared_ptr<CameraNode> camera) { m_Camera = camera; }
	const shared_ptr<CameraNode> GetCamera() const { return m_Camera; }


	void PushAndSetMatrix(const Mat4x4 &toWorld)
	{
		

		m_MatrixStack->Push();
		m_MatrixStack->MultMatrixLocal(&toWorld);
		Mat4x4 mat = GetTopMatrix();
		m_Renderer->VSetWorldTransform(&mat);
	}

	void PopMatrix()
	{
		
		m_MatrixStack->Pop();
		Mat4x4 mat = GetTopMatrix();
		m_Renderer->VSetWorldTransform(&mat);
	}

	const Mat4x4 GetTopMatrix()
	{
		
		return static_cast<const Mat4x4>(*m_MatrixStack->GetTop());
	}

	LightManager *GetLightManager() { return m_LightManager; }

	void AddAlphaSceneNode(AlphaSceneNode *asn) { m_AlphaSceneNodes.push_back(asn); }

	HRESULT Pick(RayCast *pRayCast) { return m_Root->VPick(this, pRayCast); }

	shared_ptr<IRenderer> GetRenderer() { return m_Renderer; }
};