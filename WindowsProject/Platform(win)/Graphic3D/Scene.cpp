#include"GameCodeStd.h"
#include"GameCode\GameCode.h"
#include"EventManager\EventManager.h"
#include"EventManager\Events.h"
#include"Utilities\String.h"
#include"geometry.h"
#include"light.h"


Scene::Scene(shared_ptr<IRenderer> renderer)
{
	m_Root.reset(New RootNode());
	m_Renderer = renderer;
	m_LightManager = New LightManager;

	D3DXCreateMatrixStack(0, &m_MatrixStack);

	
	IEventManager* pEventMgr = IEventManager::Get();
	pEventMgr->VAddListener(MakeDelegate(this, &Scene::NewRenderComponentDelegate), EvtData_New_Render_Component::s_EventType);
	pEventMgr->VAddListener(MakeDelegate(this, &Scene::DestroyActorDelegate), EvtData_Destroy_Actor::s_EventType);
	pEventMgr->VAddListener(MakeDelegate(this, &Scene::MoveActorDelegate), EvtData_Move_Actor::s_EventType);
	pEventMgr->VAddListener(MakeDelegate(this, &Scene::ModifiedRenderComponentDelegate), EvtData_Modified_Render_Component::s_EventType);
}


Scene::~Scene()
{
	
	IEventManager* pEventMgr = IEventManager::Get();
	pEventMgr->VRemoveListener(MakeDelegate(this, &Scene::NewRenderComponentDelegate), EvtData_New_Render_Component::s_EventType);
	pEventMgr->VRemoveListener(MakeDelegate(this, &Scene::DestroyActorDelegate), EvtData_Destroy_Actor::s_EventType);
	pEventMgr->VRemoveListener(MakeDelegate(this, &Scene::MoveActorDelegate), EvtData_Move_Actor::s_EventType);

	pEventMgr->VRemoveListener(MakeDelegate(this, &Scene::ModifiedRenderComponentDelegate), EvtData_Modified_Render_Component::s_EventType);

	SAFE_RELEASE(m_MatrixStack);
	SAFE_DELETE(m_LightManager);
}




HRESULT Scene::OnRender()
{
	// The Rendering sequence
	// 1. Static objects & terrain
	// 2. Actors (dynamic objects that can move)
	// 3. The Sky
	// 4. Anything with Alpha

	if (m_Root && m_Camera)
	{

		m_Camera->SetViewTransform(this);

		m_LightManager->CalcLighting(this);

		if (m_Root->VPreRender(this) == S_OK)
		{
			m_Root->VRender(this);
			m_Root->VRenderChildren(this);
			m_Root->VPostRender(this);
		}
		RenderAlphaPass();
	}

	return S_OK;
}



HRESULT Scene::OnLostDevice()
{
	if (m_Root)
	{
		return m_Root->VOnLostDevice(this);
	}
	return S_OK;
}




HRESULT Scene::OnRestore()
{
	if (!m_Root)
		return S_OK;

	HRESULT hr;
	V_RETURN(m_Renderer->VOnRestore());

	return m_Root->VOnRestore(this);
}





bool Scene::AddChild(ActorId id, shared_ptr<ISceneNode> kid)
{
	if (id != INVALID_ACTOR_ID)
	{
		// This allows us to search for this later based on actor id
		m_ActorMap[id] = kid;
	}

	shared_ptr<LightNode> pLight = dynamic_pointer_cast<LightNode>(kid);
	if (pLight != NULL && m_LightManager->m_Lights.size() + 1 < MAXIMUM_LIGHTS_SUPPORTED)
	{
		m_LightManager->m_Lights.push_back(pLight);
	}
	return m_Root->VAddChild(kid);
}

bool Scene::RemoveChild(ActorId id)
{
	if (id == INVALID_ACTOR_ID)
		return false;
	shared_ptr<ISceneNode> kid = FindActor(id);
	shared_ptr<LightNode> pLight = dynamic_pointer_cast<LightNode>(kid);
	if (pLight != NULL)
	{
		m_LightManager->m_Lights.remove(pLight);
	}
	m_ActorMap.erase(id);
	return m_Root->VRemoveChild(id);
}



void Scene::NewRenderComponentDelegate(IEventDataPtr pEventData)
{
	shared_ptr<EvtData_New_Render_Component> pCastEventData = static_pointer_cast<EvtData_New_Render_Component>(pEventData);

	ActorId actorId = pCastEventData->GetActorId();
	shared_ptr<SceneNode> pSceneNode(pCastEventData->GetSceneNode());

		
	if (FAILED(pSceneNode->VOnRestore(this)))
	{
		std::string error = "Failed to restore scene node to the scene for actorid " + ToStr(actorId);
		ERROR(error);
		return;
	}

	AddChild(actorId, pSceneNode);
}

void Scene::ModifiedRenderComponentDelegate(IEventDataPtr pEventData)
{
	shared_ptr<EvtData_Modified_Render_Component> pCastEventData = static_pointer_cast<EvtData_Modified_Render_Component>(pEventData);

	ActorId actorId = pCastEventData->GetActorId();
	if (actorId == INVALID_ACTOR_ID)
	{
		ERROR("Scene::ModifiedRenderComponentDelegate - unknown actor id!");
		return;
	}

	if (g_pApp->GetGameLogic()->GetState() == BGS_LOADING)
		return;

	shared_ptr<ISceneNode> pSceneNode = FindActor(actorId);
	
	if (!pSceneNode || FAILED(pSceneNode->VOnRestore(this)))
	{
		ERROR("Failed to restore scene node to the scene for actorid " + ToStr(actorId));
	}
}

void Scene::DestroyActorDelegate(IEventDataPtr pEventData)
{
	shared_ptr<EvtData_Destroy_Actor> pCastEventData = static_pointer_cast<EvtData_Destroy_Actor>(pEventData);
	RemoveChild(pCastEventData->GetActorId());
}

void Scene::MoveActorDelegate(IEventDataPtr pEventData)
{
	shared_ptr<EvtData_Move_Actor> pCastEventData = static_pointer_cast<EvtData_Move_Actor>(pEventData);

	ActorId id = pCastEventData->GetActorId();
	Mat4x4 transform = pCastEventData->GetMatrix();

	shared_ptr<ISceneNode> pNode = FindActor(id);
	if (pNode)
	{
		pNode->VSetTransform(&transform);
	}
}




HRESULT Scene::OnUpdate(const int deltaMilliseconds)
{
	if (!m_Root)
		return S_OK;

	static DWORD lastTime = timeGetTime();
	DWORD elapsedTime = 0;
	DWORD now = timeGetTime();

	elapsedTime = now - lastTime;
	lastTime = now;

	return m_Root->VOnUpdate(this, elapsedTime);
}



shared_ptr<ISceneNode> Scene::FindActor(ActorId id)
{
	SceneActorMap::iterator i = m_ActorMap.find(id);
	if (i == m_ActorMap.end())
	{
		return shared_ptr<ISceneNode>();
	}

	return i->second;
}



void Scene::RenderAlphaPass()
{
	shared_ptr<IRenderState> alphaPass = m_Renderer->VPrepareAlphaPass();

	m_AlphaSceneNodes.sort();
	while (!m_AlphaSceneNodes.empty())
	{
		AlphaSceneNodes::reverse_iterator i = m_AlphaSceneNodes.rbegin();
		PushAndSetMatrix((*i)->m_Concat);
		(*i)->m_pNode->VRender(this);
		delete (*i);
		PopMatrix();
		m_AlphaSceneNodes.pop_back();
	}
}