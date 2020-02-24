#pragma once
#include "GameCodeStd.h"

#include <mmsystem.h>

#include"GameCode.h"
#include"interface.h"
#include "AI\Pathing.h"
#include "EventManager\EventManager.h"				
#include "MainLoop\Initialization.h"			
#include "MainLoop\Process.h"
#include "Network\Network.h"
#include "ResourceCache\ResCache.h"
#include "Physics\Physics.h"
#include "Actors\Actor.h"
#include "../Actors/ActorFactory.h"
#include "Utilities\String.h"
#include "UI\HumanView.h"  
#include "BaseGameLogic.h"
#include"MainLoop\ProcessManager.h"
#include"Utilities\Math.h"
#include"EventManager\EventManager.h"
#include"Actors\Actor.h"
#include"interface.h"


class BaseGameLogic;
class LevelManager;


enum BaseGameState
{
	BGS_INVALID,
	BGS_INITIALIZING,
	BGS_MAIN_MENU,
	BGS_WAITING_FOR_PLAYERS,
	BGS_LOADING,
	BGS_WAITING_FOR_PLAYERS_TO_LOAD,
	BGS_SPAWNING_PLAYERS_ACTORS,
	BGS_RUNNING

};


typedef std::map<ActorId, StrongActorPtr> ActorMap;

class BaseGameLogic : public IGameLogic
{
	friend class GameCodeApp;

protected:
	float m_LifeTime;//How long the game in session
	ProcessManager* m_pProcessManager;
	Random m_Random;
	ActorMap m_Actors;
	ActorId m_LastActorId;
	BaseGameState m_State;
	int m_ExpectedPlayers; //Local Players
	int m_ExpectedRemotePlayers;
	int m_NumAIs;
	int m_HumanPlayersAttached;
	int m_AIPlayersAttached;
	int m_HumanGamesLoaded;
	GameViewList m_GameViews;
	shared_ptr<PathingGraph> m_pPathingGraph;
	ActorFactory* m_pActorFactory;
	bool m_IsProxy;
	int m_RemotePlayerId; //Socket number for local player
	bool m_RenderDiagnostics;//Is Running rendering diagnostics?
	shared_ptr<IGamePhysics> m_pPhysics;
	LevelManager* m_pLevelManager;

public:
	BaseGameLogic();
	virtual ~BaseGameLogic();
	bool Init();

	void SetProxy(bool isProxy) { m_IsProxy = isProxy; }
	const bool IsProxy() const { return m_IsProxy; }

	//Game state setter and getter
	const BaseGameState GetState() const { return m_State; }
	virtual void VChangeState(BaseGameState newState);

	const bool CanRunLua() const { return !IsProxy() || GetState() != BGS_RUNNING; }

	ActorId GetNewActorID() { return ++m_LastActorId; }
	shared_ptr<PathingGraph> GetPathingGraph() { return m_pPathingGraph; }
	Random& GetRandomGenerator() { return m_Random; }

	virtual void VAddView(shared_ptr<IGameView> pView, ActorId actorId = INVALID_ACTOR_ID);
	virtual void VRemoveView(shared_ptr<IGameView> pView);

	virtual WeakActorPtr VGetActor(const ActorId actorId);
	virtual StrongActorPtr VCreateActor(const std::string &actorResource, TiXmlElement* overrides, const Mat4x4* initialTransform=NULL, const ActorId serversActorId= INVALID_ACTOR_ID);
	virtual void VDestroyActor(const ActorId actorId);
	virtual void VModifyActor(const ActorId id, TiXmlElement* overrides);
	virtual void VMoveActor(const ActorId id, const Mat4x4& mat) {}

	//For editor
	std::string GetActorXml(const ActorId id);

	//Level management
	const LevelManager* GetLevelManager() { return m_pLevelManager; }
	virtual bool VLoadGame(const char* levelResource) override;
	virtual void VSetProxy();

	//Update
	virtual void VOnUpdate(float time, float elapsedTime);

	//Render Diagnostics
	void ToggleRenderDiagnostics() { m_RenderDiagnostics = !m_RenderDiagnostics; }
	virtual void VRenderDiagnostics();
	virtual shared_ptr<IGamePhysics> VGetGamePhysics() { return m_pPhysics; }

	void AttachProcess(StrongProcessPtr pProcess) { if (m_pProcessManager) m_pProcessManager->AttachProcess(pProcess); }

	void RequestDestroyActorDelegate(IEventDataPtr pEventData);

protected:
	virtual ActorFactory* VCreateActorFactory();

	virtual bool VLoadGameDelegate(TiXmlElement* pLevelData) { return true; }

	void MoveActorDelegate(IEventDataPtr pEventData);
	void RequestNewActorDelegate(IEventDataPtr pEventData);







};