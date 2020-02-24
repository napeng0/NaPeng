#pragma once
#include<SDKmesh.h>
#include"geometry.h"
#include"ResourceCache\ResCache.h"
#include"Actors\RenderComponent.h"
#include"SceneNode.h"


class D3DSdkMeshResourceExtraData11 : public IResourceExtraData
{
	friend class SdkMeshResourceLoader;

public:
	D3DSdkMeshResourceExtraData11() { };
	virtual ~D3DSdkMeshResourceExtraData11() { }
	virtual std::string VToString() { return "D3DSdkMeshResourceExtraData11"; }

	CDXUTSDKMesh                m_Mesh11;
};



class SdkMeshResourceLoader : public IResourceLoader
{
public:
	virtual bool VUseRawFile() { return false; }
	virtual bool VDiscardRawBufferAfterLoad() { return false; }
	virtual unsigned int VGetLoadedResourceSize(char *rawBuffer, unsigned int rawSize);
	virtual bool VLoadResource(char *rawBuffer, unsigned int rawSize, shared_ptr<ResHandle> handle);
	virtual std::string VGetPattern() { return "*.sdkmesh"; }
};


class D3DMeshNode9 : public D3DSceneNode9
{
protected:
	ID3DXMesh *m_pMesh;
	std::wstring m_XFileName;

public:
	D3DMeshNode9(const ActorId actorId,
		WeakBaseRenderComponentPtr renderComponent,
		ID3DXMesh *Mesh,
		RenderPass renderPass,
		const Mat4x4 *t);

	D3DMeshNode9(const ActorId actorId,
		WeakBaseRenderComponentPtr renderComponent,
		std::wstring xFileName,
		RenderPass renderPass,
		const Mat4x4 *t);

	virtual ~D3DMeshNode9() { SAFE_RELEASE(m_pMesh); }
	HRESULT VRender(Scene *pScene);
	virtual HRESULT VOnRestore(Scene *pScene);
	virtual HRESULT VPick(Scene *pScene, RayCast *pRayCast);

	float CalcBoundingSphere();
};



class D3DShaderMeshNode9 : public D3DMeshNode9
{
protected:
	ID3DXEffect *m_pEffect;
	std::string m_fxFileName;

public:
	D3DShaderMeshNode9(const ActorId actorId,
		WeakBaseRenderComponentPtr renderComponent,
		ID3DXMesh *Mesh,
		std::string fxFileName,
		RenderPass renderPass,
		const Mat4x4 *t);

	D3DShaderMeshNode9(const ActorId actorId,
		WeakBaseRenderComponentPtr renderComponent,
		std::wstring xFileName,
		std::string fxFileName,
		RenderPass renderPass,
		const Mat4x4 *t);

	virtual ~D3DShaderMeshNode9() { SAFE_RELEASE(m_pEffect); }
	virtual HRESULT VOnRestore(Scene *pScene);
	virtual HRESULT VOnLostDevice(Scene *pScene);
	HRESULT VRender(Scene *pScene);
};



class D3DTeapotMeshNode9 : public D3DShaderMeshNode9
{
public:
	D3DTeapotMeshNode9(const ActorId actorId, WeakBaseRenderComponentPtr renderComponent, std::string fxFileName, RenderPass renderPass, const Mat4x4 *t);
	virtual HRESULT VOnRestore(Scene *pScene);
};


class D3DShaderMeshNode11 : public SceneNode
{
public:
	D3DShaderMeshNode11(const ActorId actorId,
		WeakBaseRenderComponentPtr renderComponent,
		std::string sdkMeshFileName,
		RenderPass renderPass,
		const Mat4x4 *t);

	virtual HRESULT VOnRestore(Scene *pScene);
	virtual HRESULT VOnLostDevice(Scene *pScene) { return S_OK; }
	virtual HRESULT VRender(Scene *pScene);
	virtual HRESULT VPick(Scene *pScene, RayCast *pRayCast);

protected:
	std::string					m_sdkMeshFileName;

	VertexShader		m_VertexShader;
	PixelShader		m_PixelShader;

	float CalcBoundingSphere(CDXUTSDKMesh *mesh11);			// this was added post press.
};



class D3DTeapotMeshNode11 : public D3DShaderMeshNode11
{
public:
	D3DTeapotMeshNode11(const ActorId actorId, WeakBaseRenderComponentPtr renderComponent, RenderPass renderPass, const Mat4x4 *t)
		: D3DShaderMeshNode11(actorId, renderComponent, "art\\teapot.sdkmesh", renderPass, t) { }
	bool VIsVisible() { return true; }
};