
#include "BaseGameLogic.h"






class PathingGraph;
class ActorFactory;
class LevelManager;


typedef std::string Level;

class LevelManager
{
protected:
	std::vector<Level> m_Levels;
	int m_CurrentLevel;

public:
	const std::vector<Level>& GetLevels() const { return m_Levels; }
	const int GetCurrrentLevel() const { return m_CurrentLevel; }
	bool Initialize(const std::vector<std::string>& levels);
};





BaseGameLogic::BaseGameLogic()
{
	m_LastActorId = 0;
	m_LifeTime = 0;
	m_pProcessManager = New ProcessManager;
	m_Random.Randomize();
	m_State = BGS_INITIALIZING;
	m_IsProxy = false;
	m_RenderDiagnostics = false;
	m_ExpectedPlayers = 0;
	m_ExpectedRemotePlayers = 0;
	m_NumAIs = 0;
	m_HumanPlayersAttached = 0;
	m_AIPlayersAttached = 0;
	m_HumanGamesLoaded = 0;
	m_pPathingGraph = NULL;
	m_pActorFactory = NULL;

	m_pLevelManager = New LevelManager;
	ASSERT(m_pProcessManager && m_pLevelManager);
	m_pLevelManager->Initialize(g_pApp->m_ResCache->Match("world\\*.xml"));

	
	RegisterEngineScriptEvents();
}

BaseGameLogic::~BaseGameLogic()
{
	
	while (!m_GameViews.empty())
		m_GameViews.pop_front();

	SAFE_DELETE(m_pLevelManager);
	SAFE_DELETE(m_pProcessManager);
	SAFE_DELETE(m_pActorFactory);

	// Destroy all actors
	for (auto it = m_Actors.begin(); it != m_Actors.end(); ++it)
		it->second->Destroy();
	m_Actors.clear();

	IEventManager::Get()->VRemoveListener(MakeDelegate(this, &BaseGameLogic::RequestDestroyActorDelegate), EvtData_Request_Destroy_Actor::s_EventType);
}

bool BaseGameLogic::Init(void)
{
	m_pActorFactory = VCreateActorFactory();
	m_pPathingGraph.reset(CreatePathingGraph());

	IEventManager::Get()->VAddListener(MakeDelegate(this, &BaseGameLogic::RequestDestroyActorDelegate), EvtData_Request_Destroy_Actor::s_EventType);

	return true;
}

std::string BaseGameLogic::GetActorXml(const ActorId id)
{
	StrongActorPtr pActor = MakeStrongPtr(VGetActor(id));
	if (pActor)
		return pActor->ToXML();
	else
		ERROR("Couldn't find actor: " + ToStr(id));

	return std::string();
}

bool BaseGameLogic::VLoadGame(const char* levelResource)
{
	// Grab the root XML node
	TiXmlElement* pRoot = XmlResourceLoader::LoadAndReturnRootXmlElement(levelResource);
	if (!pRoot)
	{
		ERROR("Failed to find level resource file: " + std::string(levelResource));
		return false;
	}

	// Pre and post load scripts
	const char* preLoadScript = NULL;
	const char* postLoadScript = NULL;

	// Parse the pre & post script attributes
	TiXmlElement* pScriptElement = pRoot->FirstChildElement("Script");
	if (pScriptElement)
	{
		preLoadScript = pScriptElement->Attribute("preLoad");
		postLoadScript = pScriptElement->Attribute("postLoad");
	}

	// Load the pre-load script if there is one
	if (preLoadScript)
	{
		Resource resource(preLoadScript);
		shared_ptr<ResHandle> pResourceHandle = g_pApp->m_ResCache->GetHandle(&resource);  // this actually loads the XML file from the zip file
	}

	// Load all initial actors
	TiXmlElement* pActorsNode = pRoot->FirstChildElement("StaticActors");
	if (pActorsNode)
	{
		for (TiXmlElement* pNode = pActorsNode->FirstChildElement(); pNode; pNode = pNode->NextSiblingElement())
		{
			const char* actorResource = pNode->Attribute("resource");

			StrongActorPtr pActor = VCreateActor(actorResource, pNode);
			if (pActor)
			{
				// Fire an event letting everyone else know that we created a new actor
				shared_ptr<EvtData_New_Actor> pNewActorEvent(New EvtData_New_Actor(pActor->GetActorId()));
				IEventManager::Get()->VQueueEvent(pNewActorEvent);
			}
		}
	}

	// Initialize all human views
	for (auto it = m_GameViews.begin(); it != m_GameViews.end(); ++it)
	{
		shared_ptr<IGameView> pView = *it;
		if (pView->VGetType() == GV_HUMAN)
		{
			shared_ptr<HumanView> pHumanView = static_pointer_cast<HumanView, IGameView>(pView);
			pHumanView->LoadGame(pRoot);
		}
	}


	// Call the delegate load function
	if (!VLoadGameDelegate(pRoot))
		return false;  

	// Load the post-load script if there is one
	if (postLoadScript)
	{
		Resource resource(postLoadScript);
		shared_ptr<ResHandle> pResourceHandle = g_pApp->m_ResCache->GetHandle(&resource);  
	}

	// Trigger the Environment Loaded Game event - only then can player actors and AI be spawned!
	if (m_IsProxy)
	{
		shared_ptr<EvtData_Remote_Environment_Loaded> pNewGameEvent(New EvtData_Remote_Environment_Loaded);
		IEventManager::Get()->VTriggerEvent(pNewGameEvent);
	}
	else
	{
		shared_ptr<EvtData_Environment_Loaded> pNewGameEvent(New EvtData_Environment_Loaded);
		IEventManager::Get()->VTriggerEvent(pNewGameEvent);
	}
	return true;
}



void BaseGameLogic::VSetProxy()
{
	m_IsProxy = true;

	IEventManager::Get()->VAddListener(MakeDelegate(this, &BaseGameLogic::RequestNewActorDelegate), EvtData_Request_New_Actor::s_EventType);

	m_pPhysics.reset(CreateNullPhysics());
}


StrongActorPtr BaseGameLogic::VCreateActor(const std::string &actorResource, TiXmlElement* overrides, const Mat4x4* initialTransform, const ActorId serversActorId)
{
	ASSERT(m_pActorFactory);
	if (!m_IsProxy && serversActorId != INVALID_ACTOR_ID)
		return StrongActorPtr();

	if (m_IsProxy && serversActorId == INVALID_ACTOR_ID)
		return StrongActorPtr();

	StrongActorPtr pActor = m_pActorFactory->CreateActor(actorResource.c_str(), overrides, initialTransform, serversActorId);
	if (pActor)
	{
		m_Actors.insert(std::make_pair(pActor->GetActorId(), pActor));
		if (!m_IsProxy && (m_State == BGS_SPAWNING_PLAYERS_ACTORS || m_State == BGS_RUNNING))
		{
			shared_ptr<EvtData_Request_New_Actor> pNewActor(New EvtData_Request_New_Actor(actorResource, initialTransform, pActor->GetActorId()));
			IEventManager::Get()->VTriggerEvent(pNewActor);
		}
		return pActor;
	}
	else
	{
		return StrongActorPtr();
	}
}

void BaseGameLogic::VDestroyActor(const ActorId actorId)
{
	// We need to trigger a synchronous event to ensure that any systems responding to this event can still access a 
	// valid actor if need be.  The actor will be destroyed after this.
	shared_ptr<EvtData_Destroy_Actor> pEvent(New EvtData_Destroy_Actor(actorId));
	IEventManager::Get()->VTriggerEvent(pEvent);

	auto findIt = m_Actors.find(actorId);
	if (findIt != m_Actors.end())
	{
		findIt->second->Destroy();
		m_Actors.erase(findIt);
	}
}

WeakActorPtr BaseGameLogic::VGetActor(const ActorId actorId)
{
	ActorMap::iterator findIt = m_Actors.find(actorId);
	if (findIt != m_Actors.end())
		return findIt->second;
	return WeakActorPtr();
}

void BaseGameLogic::VModifyActor(const ActorId actorId, TiXmlElement* overrides)
{
	ASSERT(m_pActorFactory);
	if (!m_pActorFactory)
		return;

	auto findIt = m_Actors.find(actorId);
	if (findIt != m_Actors.end())
	{
		m_pActorFactory->ModifyActor(findIt->second, overrides);
	}
}

void BaseGameLogic::VOnUpdate(float time, float elapsedTime)
{
	int deltaMilliseconds = int(elapsedTime * 1000.0f);
	m_LifeTime += elapsedTime;

	switch (m_State)
	{
	case BGS_INITIALIZING:
		VChangeState(BGS_MAIN_MENU);
		break;

	case BGS_MAIN_MENU:
		break;

	case BGS_LOADING:
		
		break;

	case BGS_WAITING_FOR_PLAYERS_TO_LOAD:
		if (m_ExpectedPlayers + m_ExpectedRemotePlayers <= m_HumanGamesLoaded)
		{
			VChangeState(BGS_SPAWNING_PLAYERS_ACTORS);
		}
		break;

	case BGS_SPAWNING_PLAYERS_ACTORS:
		VChangeState(BGS_RUNNING);
		break;

	case BGS_WAITING_FOR_PLAYERS:
		if (m_ExpectedPlayers + m_ExpectedRemotePlayers == m_HumanPlayersAttached)
		{
			// The server sends us the level name as a part of the login message. 
			// We have to wait until it arrives before loading the level
			if (!g_pApp->m_Options.m_Level.empty())
			{
				VChangeState(BGS_LOADING);
			}
		}
		break;

	case BGS_RUNNING:
		m_pProcessManager->UpdateProcesses(deltaMilliseconds);

		if (m_pPhysics && !m_IsProxy)
		{
			m_pPhysics->VOnUpdate(elapsedTime);
			m_pPhysics->VSyncVisibleScene();
		}

		break;

	default:
		ERROR("Unrecognized state.");
	}

	// Update all game views
	for (GameViewList::iterator it = m_GameViews.begin(); it != m_GameViews.end(); ++it)
	{
		(*it)->VOnUpdate(deltaMilliseconds);
	}

	// Update game actors
	for (ActorMap::const_iterator it = m_Actors.begin(); it != m_Actors.end(); ++it)
	{
		it->second->Update(deltaMilliseconds);
	}

}


void BaseGameLogic::VChangeState(BaseGameState newState)
{
	if (newState == BGS_WAITING_FOR_PLAYERS)
	{
		//Remove Main menu
		m_GameViews.pop_front();


		m_ExpectedPlayers = 1;
		m_ExpectedRemotePlayers = g_pApp->m_Options.m_ExpectedPlayers - 1;
		m_NumAIs = g_pApp->m_Options.m_NumAIs;

		if (!g_pApp->m_Options.m_GameHost.empty())
		{
			VSetProxy();
			m_NumAIs = 0;				
			m_ExpectedRemotePlayers = 0;	

			if (!g_pApp->AttachAsClient())
			{
				// Open main menu
				VChangeState(BGS_MAIN_MENU);
				return;
			}
		}
		else if (m_ExpectedRemotePlayers > 0)
		{
			BaseSocketManager *pServer = New BaseSocketManager();
			if (!pServer->Init())
			{
				// Open main menu
				VChangeState(BGS_MAIN_MENU);
				return;
			}

			pServer->AddSocket(new GameServerListenSocket(g_pApp->m_Options.m_ListenPort));
			g_pApp->m_pBaseSocketManager = pServer;
		}
	}
	else if (newState == BGS_LOADING)
	{
		m_State = newState;
		if (!g_pApp->VLoadGame())
		{
			ERROR("The game failed to load.");
			g_pApp->AbortGame();
		}
		else
		{
			VChangeState(BGS_WAITING_FOR_PLAYERS_TO_LOAD);			// we must wait for all human player to report their environments are loaded.
			return;
		}
	}

	m_State = newState;
}



void BaseGameLogic::VRenderDiagnostics()
{
	if (m_RenderDiagnostics)
	{
		m_pPhysics->VRenderDiagnostics();
	}
}


void BaseGameLogic::VAddView(shared_ptr<IGameView> pView, ActorId actorId)
{
	// This makes sure that all views have a non-zero view id.
	int viewId = static_cast<int>(m_GameViews.size());
	m_GameViews.push_back(pView);
	pView->VOnAttach(viewId, actorId);
	pView->VOnRestore();
}


void BaseGameLogic::VRemoveView(shared_ptr<IGameView> pView)
{
	m_GameViews.remove(pView);
}


ActorFactory* BaseGameLogic::VCreateActorFactory(void)
{
	return New ActorFactory;
}



void BaseGameLogic::RequestDestroyActorDelegate(IEventDataPtr pEventData)
{
	shared_ptr<EvtData_Request_Destroy_Actor> pCastEventData = static_pointer_cast<EvtData_Request_Destroy_Actor>(pEventData);
	VDestroyActor(pCastEventData->GetActorId());
}





void BaseGameLogic::MoveActorDelegate(IEventDataPtr pEventData)
{
	shared_ptr<EvtData_Move_Actor> pCastEventData = static_pointer_cast<EvtData_Move_Actor>(pEventData);
	VMoveActor(pCastEventData->GetActorId(), pCastEventData->GetMatrix());
}

void BaseGameLogic::RequestNewActorDelegate(IEventDataPtr pEventData)
{
	// This should only happen if the game logic is a proxy, and there's a server asking us to create an actor.
	ASSERT(m_IsProxy);
	if (!m_IsProxy)
		return;

	shared_ptr<EvtData_Request_New_Actor> pCastEventData = static_pointer_cast<EvtData_Request_New_Actor>(pEventData);

	// Create the actor
	StrongActorPtr pActor = VCreateActor(pCastEventData->GetActorResource(), NULL, pCastEventData->GetInitialTransform(), pCastEventData->GetServerActorId());
	if (pActor)
	{
		shared_ptr<EvtData_New_Actor> pNewActorEvent(New EvtData_New_Actor(pActor->GetActorId(), pCastEventData->GetViewId()));
		IEventManager::Get()->VQueueEvent(pNewActorEvent);
	}
}