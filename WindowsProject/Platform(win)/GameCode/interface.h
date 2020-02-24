#pragma once
#include"Utilities\templates.h"
#include<winnt.h>
#include<minwindef.h>
#include"GameCodeStd.h"
#include"BaseGameLogic.h"



//Interfaces of some basic classes

//Interfaces related to game logic
class Actor;
class ActorComponent;

typedef unsigned int ActorId;
typedef unsigned int ComponentId;

const ActorId INVALID_ACTOR_ID = 0;
const ComponentId INVALID_COMPONENT_ID = 0;

using std::shared_ptr;
using std::weak_ptr;
typedef shared_ptr<Actor> StrongActorPtr;
typedef weak_ptr<Actor> WeakActorPtr;
typedef shared_ptr<ActorComponent> StrongActorComponentPtr;
typedef weak_ptr<ActorComponent> WeakActotComponentPtr;

template<class T>
struct SortBy_SharedPtr_Content
{
	bool operator()(const shared_ptr<T>& lhs, const shared_ptr<T>& rhs) const
	{
		return (*lhs < *rhs);
	}
};


class IScreenElement
{
public:
	virtual HRESULT VOnRestore() = 0;
	virtual HRESULT VOnLostDevice() = 0;
	virtual HRESULT VOnRender() = 0;
	virtual void VOnUpdate() = 0;
	virtual int VGetZOrder() const = 0;
	virtual void VSetZOrder(const int zOrder) = 0;
	virtual bool VIsVisible() const = 0;
	virtual void VSetVisible(bool isVisible) = 0;
	virtual LRESULT CALLBACK VOnMsgProc(AppMsg msg) = 0;

	virtual ~IScreenElement() {};
	virtual const bool operator<(const IScreenElement& other)
	{
		return (this->VGetZOrder() < other.VGetZOrder);
	}

};


class IGamePhysics;

class IGameLogic
{
public:
	virtual WeakActorPtr VGetActor(const ActorId id) = 0;
	virtual StrongActorPtr VCreateActor(const std::string& actorResource, TiXmlElement* overrides, const Mat4x4* initialTransform = NULL, const ActorId serversActorId = INVALID_ACTOR_ID) = 0;
	virtual void VDestroyActor(const ActorId actorId) = 0;
	virtual bool VLoadGame(const char* levelResource) = 0;
	virtual void VSetProxy() = 0;
	virtual void VOnUpdate(float delta, float elapsedTime) = 0;
	virtual void VChangeState(enum BaseGameState newState) = 0;
	virtual void VMoveActor(const ActorId id, const Mat4x4& mat) = 0;
	virtual shared_ptr<IGamePhysics> VGetGamePhysics() = 0;
};


enum GameViewType
{
	GV_HUMAN,
	GV_REMOTE,
	GV_AI,
	GV_RECORDER,
	GV_OTHER
};
typedef unsigned int GameViewId;
extern const GameViewId InvalidGameViewId;

class IGameView
{
public:
	virtual HRESULT VOnRestore() = 0;
	virtual void VOnRender(double time, float elapsedTime) = 0;
	virtual HRESULT VOnLostDevice() = 0;
	virtual GameViewType VGetType() = 0;
	virtual GameViewId VGetId() const = 0;
	virtual void VOnAttach(GameViewId vId, ActorId aId) = 0;
	virtual LRESULT CALLBACK VOnMsgProc(AppMsg msg) = 0;
	virtual void VOnUpdate(unsigned long deltaMs) = 0;

	virtual ~IGameView() {};
};


typedef std::list<shared_ptr<IScreenElement>> ScreenElementList;
typedef std::list<shared_ptr<IGameView>> GameViewList;


//Interfaces related to input devices
class IKeyboardHandler
{
public:
	virtual bool VOnKeyDown(const BYTE c) = 0;
	virtual bool VOnKeyUp(const BYTE c) = 0;
};

class IPointerHandler
{
public:
	virtual bool VOnPointerMove(const Point& pos, const int radius) = 0;
	virtual bool VOnPointerButtonDown(const Point& pos, const int radius, const std::string& buttonName) = 0;
	virtual bool VOnPointerButtonUp(const Point& pos, const int radius, const std::string& buttonName) = 0;
};



//Interfaces related to resource cache
class Resource;
class IResourceFile;
class ResHandle;

class IResourceLoader
{
public:
	virtual std::string VGetPattern() = 0;
	virtual bool VUseRawFile() = 0;
	virtual bool VDiscardRawBufferAfterLoad() = 0;
	virtual bool VAddNullZero()
	{
		return false;
	}
	virtual unsigned int VGetLoadedResourceSize(char* rawBuffer, unsigned int rawSize) = 0;
	virtual bool VLoadResource(char* rawBuffer, unsigned int rawSize, shared_ptr<ResHandle> pHandle) = 0;

};


class IResourceFile
{
public:
	virtual bool VOpen() = 0;
	virtual int VGetRawResourceSize(const Resource& res) = 0;
	virtual int VGetRawResource(const Resource& res, char* buffer) = 0;
	virtual int VGetNumResources() const = 0;
	virtual std::string VGetResourceName(int num) const = 0;
	virtual bool VIsUsingDevelopmentDirectories() const = 0;
	virtual ~IResourceFile() {};

};


//Interfaces related to 3D scenes rendering
enum RenderPass
{
	RENDERPASS_0,
	RENDERPASS_STATIC=RENDERPASS_0,
	RENDERPASS_ACTOR,
	RENDERPASS_SKY,
	RENDERPASS_NOT_RENDERED,
	RENDERPASS_LAST

};


class Scene;
class SceneNodeProperties;
class RayCast;
class LightNode;

typedef std::list<shared_ptr<LightNode>> Lights;


class IRenderState
{
public:
	virtual std::string VToString() = 0;
};

class IRenderer
{
public:
	virtual void VSetBackgroundColor(BYTE bgA, BYTE bgR, BYTE bgG, BYTE bgB) = 0;
	virtual HRESULT VOnRestore() = 0;
	virtual void VShutDown() = 0;
	virtual bool VPreRender() = 0;
	virtual bool VPostRender() = 0;
	virtual void VCalcLighting(Lights* lights, int maximumLights) = 0;
	virtual void VSetWorldTransform(const Mat4x4* mat) = 0;
	virtual void VSetViewTransform(const Mat4x4* mat) = 0;
	virtual void VSetProjectionTransform(const Mat4x4* mat) = 0;
	virtual shared_ptr<IRenderState> VPrepareAlphaPass() = 0;
	virtual shared_ptr<IRenderState> VPrepareSkyBoxPass() = 0;
	virtual void VDrawLine(const Vec3& from, const Vec3& to, const Color& color) = 0;

};

class ISceneNode
{
public:
	virtual const SceneNodeProperties const* VGet() const = 0;
	virtual void VSetTransform(const Mat4x4* toWorld, const Mat4x4* fromWorld = NULL) = 0;
	virtual HRESULT VOnUpdate(Scene* pScene, const DWORD elapsedMs) = 0;
	virtual HRESULT VOnRestore(Scene* pScene) = 0;
	virtual HRESULT VPreRender(Scene* pScene) = 0;
	virtual bool VIsVisible(Scene* pScene) const = 0;
	virtual HRESULT VRender(Scene* pScene) = 0;
	virtual HRESULT VRenderChildren(Scene* pScene) = 0;
	virtual HRESULT VPostRender(Scene* pScene) = 0;
	virtual bool VAddChild(shared_ptr<ISceneNode> pkid) = 0;
	virtual bool VRemoveChild(ActorId id) = 0;
	virtual HRESULT VOnLostDevice(Scene* pScene) = 0;
	virtual HRESULT VPick(Scene* pScene, RayCast* pRayCast) = 0;

	virtual ~ISceneNode() {};
	

};


//Interface definition for a generic physics API
class IGamePhysics
{
public:
	virtual bool VInitialize() = 0;
	virtual void VSyncVisibleScene() = 0;
	virtual void VOnUpdate(float deltaS) = 0;

	//Initialization of physics objects
	virtual void VAddSphere(float radius, WeakActorPtr pactor, const std::string& desitiy, const std::string& physicsMaterial) = 0;
	virtual void VAddBox(const Vec3& dimensions, WeakActorPtr pactor, const std::string& density, const std::string& physicsMaterial) = 0;
	virtual void VAddPointCloud(Vec3* verts, int numPoints, WeakActorPtr pactor, const std::string& density, const std::string& physicsMaterial) = 0;
	virtual void VRemoveActor(ActorId id) = 0;
	
	//Debugging
	virtual void VRenderDiagnostics() = 0;
	
	//Physics
	virtual void VCreateTrigger(WeakActorPtr pActor, const Vec3& pos, const float dim) = 0;
	virtual void VApplyForce(const Vec3& dir, float newtons, ActorId id) = 0;
	virtual void VApplyTorque(const Vec3& dir, float newtons, ActorId id) = 0;
	virtual bool VKinematicMove(const Mat4x4& mat, ActorId id) = 0;

	//Physics actor states
	virtual void VRotateY(ActorId id, float angleRad, float time) = 0;
	virtual float VGetOrientationY(ActorId id) = 0;
	virtual void VStopActor(ActorId id) = 0;
	virtual Vec3 VGetVelocity(ActorId id) = 0;
	virtual void VSetVelocity(ActorId id, const Vec3& vel) = 0;
	virtual Vec3 VGetAngularVelocity(ActorId id) = 0;
	virtual void VSetAngularVelocity(ActorId id, const Vec3& vel) = 0;
	virtual void VTranslate(ActorId id, const Vec3& vec) = 0;
	virtual void VSetTransform(ActorId id, const Mat4x4& mat) = 0;
	virtual Mat4x4 VGetTransform(ActorId id) = 0;

	virtual ~IGamePhysics() {};
};


//Interface for the scripting system
class IScriptManager
{
public:
	virtual bool VInit() = 0;
	virtual void VExecuteFile(const char* resource) = 0;
	virtual void VExecuteString(const char* str) = 0;

	virtual ~IScriptManager() {};
};