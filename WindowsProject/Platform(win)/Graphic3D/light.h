#pragma once
#include"geometry.h"
#include"Scene.h"
#include"SceneNode.h"
#include"Renderer.h"

struct LightProperties
{
	float	m_Attenuation[3];  /* Attenuation coefficients */
	float	m_Range;
	float	m_Falloff;
	float	m_Theta;
	float	m_Phi;
};


class LightNode : public SceneNode
{
protected:
	LightProperties m_LightProps;

public:
	LightNode(const ActorId actorId, WeakBaseRenderComponentPtr renderComponent, const LightProperties &props, const Mat4x4 *t);
};



class D3DLightNode9 : public LightNode
{
public:
	D3DLightNode9(const ActorId actorId, WeakBaseRenderComponentPtr renderComponent, const LightProperties &lightProps, const Mat4x4 *t)
		: LightNode(actorId, renderComponent, lightProps, t) { }

	D3DLIGHT9	m_d3dLight9;

	virtual HRESULT VOnRestore(Scene *pScene);
	virtual HRESULT VOnUpdate(Scene *, DWORD const elapsedMs);
};



class D3DLightNode11 : public LightNode
{
public:
	D3DLightNode11(const ActorId actorId, WeakBaseRenderComponentPtr renderComponent, const LightProperties &lightProps, const Mat4x4 *t)
		: LightNode(actorId, renderComponent, lightProps, t) { }

	virtual HRESULT VOnRestore() { return S_OK; };
	virtual HRESULT VOnUpdate(Scene *, DWORD const elapsedMs);
};


struct ConstantBuffer_Lighting;



class LightManager
{
	friend class Scene;

protected:
	Lights					m_Lights;
	Vec4					m_vLightDir[MAXIMUM_LIGHTS_SUPPORTED];
	Color					m_vLightDiffuse[MAXIMUM_LIGHTS_SUPPORTED];
	Vec4					m_vLightAmbient;
public:
	void CalcLighting(Scene *pScene);
	void CalcLighting(ConstantBuffer_Lighting* pLighting, SceneNode *pNode);
	int GetLightCount(const SceneNode *node) { return m_Lights.size(); }
	const Vec4 *GetLightAmbient(const SceneNode *node) { return &m_vLightAmbient; }
	const Vec4 *GetLightDirection(const SceneNode *node) { return m_vLightDir; }
	const Color *GetLightDiffuse(const SceneNode *node) { return m_vLightDiffuse; }
};