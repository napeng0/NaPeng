#pragma once
#include"GameCodeStd.h"
#include"GameCode\interface.h"
#include"material.h"

class SceneNode;
class Scene;
class RayCast;
class MovementController;
class IResourceExtraData;
class ActorComponent;
class BaseRenderComponent;

typedef BaseRenderComponent* WeakBaseRenderComponentPtr;


enum AlphaType
{
	AlphaOpaque,
	AlphaTexture,
	AlphaMaterial,
	AlphaVertex
};


struct AlphaSceneNode
{
	shared_ptr<ISceneNode> m_pNode;
	Mat4x4 m_Concat;
	float m_ZOrder;

	const bool operator<(const AlphaSceneNode& other) { return m_ZOrder < other.m_ZOrder; }


};

typedef std::list<AlphaSceneNode*> AlphaSceneNodes;
typedef std::map<ActorId, shared_ptr<ISceneNode>> SceneActorMap;
typedef std::vector<shared_ptr<ISceneNode>>SceneNodeList;


class SceneNode;
class SceneNodeProperties
{
	friend class SceneNode;

protected:
	ActorId			m_ActorId;
	std::string		m_Name;
	Mat4x4			m_ToWorld, m_FromWorld;
	float			m_Radius;
	RenderPass		m_RenderPass;
	Material		m_Material;
	AlphaType		m_AlphaType;

	void SetAlpha(const float alpha)
	{
		m_AlphaType = AlphaMaterial;
		m_Material.SetAlpha(alpha);
	}
	
public:
	SceneNodeProperties();
	const ActorId& ActorId() const { return m_ActorId; }
	Mat4x4 const& ToWorld() const { return m_ToWorld; }
	Mat4x4 const& FromWorld() const { return m_FromWorld; }
	void Transform(Mat4x4* toWorld, Mat4x4* fromWorld) const;
	const char* Name() const { return m_Name.c_str(); }
	bool HasAlpha() const { return m_Material.HasAlpha(); }
	float Alpha() const { return m_Material.GetAlpha(); }
	AlphaType AlphaType() const { return m_AlphaType; }
	RenderPass RenderPass() const { return m_RenderPass; }
	float Radius() const { return m_Radius; }
	Material GetMaterial() const { return m_Material; }
			

};


typedef std::vector<shared_ptr<ISceneNode> > SceneNodeList;



class SceneNode : public ISceneNode
{
	friend class Scene;

protected:
	SceneNodeList			m_Children;
	SceneNode				*m_pParent;
	SceneNodeProperties		m_Props;
	WeakBaseRenderComponentPtr	m_RenderComponent;

public:
	SceneNode(ActorId actorId, WeakBaseRenderComponentPtr renderComponent, RenderPass renderPass, const Mat4x4 *to, const Mat4x4 *from = NULL);

	virtual ~SceneNode();

	virtual const SceneNodeProperties*  VGet() const { return &m_Props; }

	virtual void VSetTransform(const Mat4x4 *toWorld, const Mat4x4 *fromWorld = NULL);

	virtual HRESULT VOnRestore(Scene *pScene);
	virtual HRESULT VOnUpdate(Scene *, DWORD const elapsedMs);

	virtual HRESULT VPreRender(Scene *pScene);
	virtual bool VIsVisible(Scene *pScene) const;
	virtual HRESULT VRender(Scene *pScene) { return S_OK; }
	virtual HRESULT VRenderChildren(Scene *pScene);
	virtual HRESULT VPostRender(Scene *pScene);

	virtual bool VAddChild(shared_ptr<ISceneNode> kid);
	virtual bool VRemoveChild(ActorId id);
	virtual HRESULT VOnLostDevice(Scene *pScene);
	virtual HRESULT VPick(Scene *pScene, RayCast *pRayCast);

	void SetAlpha(float alpha);
	float GetAlpha() const { return m_Props.Alpha(); }

	Vec3 GetPosition() const { return m_Props.m_ToWorld.GetPosition(); }
	void SetPosition(const Vec3 &pos) { m_Props.m_ToWorld.SetPosition(pos); }

	const Vec3 GetWorldPosition() const;					

	Vec3 GetDirection() const { return m_Props.m_ToWorld.GetDirection(); }

	void SetRadius(const float radius) { m_Props.m_Radius = radius; }
	void SetMaterial(const Material &mat) { m_Props.m_Material = mat; }
};




class D3DSceneNode9 : public SceneNode
{
public:
	D3DSceneNode9(const ActorId actorId,
		WeakBaseRenderComponentPtr renderComponent,
		RenderPass renderPass,
		const Mat4x4 *t)
		: SceneNode(actorId, renderComponent, renderPass, t) { }

	virtual HRESULT VRender(Scene *pScene);
};

class D3DSceneNode11 : public SceneNode
{
public:
	virtual HRESULT VRender(Scene *pScene) { return S_OK; }
};


class CameraNode;
class SkyNode;


class RootNode : public SceneNode
{
public:
	RootNode();
	virtual bool VAddChild(shared_ptr<ISceneNode> kid);
	virtual HRESULT VRenderChildren(Scene *pScene);
	virtual bool VRemoveChild(ActorId id);
	virtual bool VIsVisible(Scene *pScene) const { return true; }
};


class CameraNode : public SceneNode
{
public:
	CameraNode(Mat4x4 const *t, Frustum const &frustum)
		: SceneNode(INVALID_ACTOR_ID, WeakBaseRenderComponentPtr(), RENDERPASS_0, t),
		m_Frustum(frustum),
		m_IsActive(true),
		m_DebugCamera(false),
		m_pTarget(shared_ptr<SceneNode>()),
		m_CamOffsetVector(0.0f, 1.0f, -10.0f, 0.0f)
	{
	}

	virtual HRESULT VRender(Scene *pScene);
	virtual HRESULT VOnRestore(Scene *pScene);
	virtual bool VIsVisible(Scene *pScene) const { return m_IsActive; }

	const Frustum &GetFrustum() { return m_Frustum; }
	void SetTarget(shared_ptr<SceneNode> pTarget)
	{
		m_pTarget = pTarget;
	}
	void ClearTarget() { m_pTarget = shared_ptr<SceneNode>(); }
	shared_ptr<SceneNode> GetTarget() { return m_pTarget; }

	Mat4x4 GetWorldViewProjection(Scene *pScene);
	HRESULT SetViewTransform(Scene *pScene);

	Mat4x4 GetProjection() { return m_Projection; }
	Mat4x4 GetView() { return m_View; }

	void SetCameraOffset(const Vec4 & cameraOffset)
	{
		m_CamOffsetVector = cameraOffset;
	}

protected:

	Frustum			m_Frustum;
	Mat4x4			m_Projection;
	Mat4x4			m_View;
	bool			m_IsActive;
	bool			m_DebugCamera;
	shared_ptr<SceneNode> m_pTarget;
	Vec4			m_CamOffsetVector;	//Direction of camera relative to target.
};


class D3DGrid9 : public SceneNode
{
protected:
	shared_ptr<ResHandle>	m_Handle;			// The resource handle for the grid texture for the axes planes
	LPDIRECT3DVERTEXBUFFER9 m_pVerts;			// The grid verts
	LPDIRECT3DINDEXBUFFER9	m_pIndices;			// The grid index
	DWORD					m_NumVerts;
	DWORD					m_NumPolys;

public:
	bool					m_TextureHasAlpha;

	D3DGrid9(ActorId actorId, WeakBaseRenderComponentPtr renderComponent, const Mat4x4* pMatrix);
	virtual ~D3DGrid9();
	virtual HRESULT VOnRestore(Scene *pScene);
	virtual HRESULT VRender(Scene *pScene);
	virtual HRESULT VPick(Scene *pScene, RayCast *pRayCast);

	bool VHasAlpha() const { return m_TextureHasAlpha; }
};



class D3DGrid11 : public SceneNode
{
protected:
	DWORD					m_NumVerts;
	DWORD					m_NumPolys;


	ID3D11Buffer*               m_pIndexBuffer;
	ID3D11Buffer*               m_pVertexBuffer;

	VertexShader		m_VertexShader;
	PixelShader		m_PixelShader;

public:
	bool					m_TextureHasAlpha;

	D3DGrid11(ActorId actorId, WeakBaseRenderComponentPtr renderComponent, const Mat4x4* pMatrix);
	virtual ~D3DGrid11();
	virtual HRESULT VOnRestore(Scene *pScene);
	virtual HRESULT VRender(Scene *pScene);
	virtual HRESULT VOnUpdate(Scene *pScene, DWORD const elapsedMs) { return S_OK; }
	virtual HRESULT VPick(Scene *pScene, RayCast *pRayCast) { return E_FAIL; }

	bool VHasAlpha() const { return m_TextureHasAlpha; }
};




class ArrowNode : public SceneNode
{
protected:
	ID3DXMesh *m_shaft;
	ID3DXMesh *m_cone;
	Mat4x4 m_coneTrans;
	Mat4x4 m_shaftTrans;

public:
	ArrowNode(std::string name, WeakBaseRenderComponentPtr renderComponent, const float length, const Mat4x4 *t, const Color &color);

	virtual ~ArrowNode() { SAFE_RELEASE(m_shaft); SAFE_RELEASE(m_cone); }
	virtual HRESULT VRender(Scene* pScene);
	virtual HRESULT VPick(Scene* pScene, RayCast* pRayCast);
};



class TestObject : public SceneNode
{
protected:
	LPDIRECT3DVERTEXBUFFER9 m_pVerts;
	DWORD					m_NumVerts;
	DWORD					m_NumPolys;

	DWORD					m_color;
	bool					m_squashed;

public:
	TestObject(std::string name);
	virtual ~TestObject();
	HRESULT VOnRestore(Scene *pScene);
	HRESULT VRender(Scene *pScene);

	static WORD g_TestObjectIndices[][3];
	static Vec3 g_CubeVerts[];
	static Vec3 g_SquashedCubeVerts[];
};