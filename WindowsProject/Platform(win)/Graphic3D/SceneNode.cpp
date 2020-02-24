#include"GameCodeStd.h"
#include"GameCode\GameCode.h"
#include"Actors\ActorComponent.h"
#include"Actors\RenderComponent.h"
#include"Actors\TransformComponent.h"
#include"Graphic3D\Renderer.h"
#include"Graphic3D\geometry.h"
#include"Graphic3D\light.h"
#include"Graphic3D\Mesh.h"
#include"Graphic3D\RayCast.h"
#include<tchar.h>
#include"Graphic3D\Shaders.h"
#include"ResourceCache\ResCache.h"
#include<xnamath.h>



SceneNodeProperties::SceneNodeProperties(void)
{
	m_ActorId = INVALID_ACTOR_ID;
	m_Radius = 0;
	m_RenderPass = RENDERPASS_0;
	m_AlphaType = AlphaOpaque;
}



void SceneNodeProperties::Transform(Mat4x4 *toWorld, Mat4x4 *fromWorld) const
{
	if (toWorld)
		*toWorld = m_ToWorld;

	if (fromWorld)
		*fromWorld = m_FromWorld;
}



SceneNode::SceneNode(ActorId actorId, WeakBaseRenderComponentPtr renderComponent, RenderPass renderPass, const Mat4x4 *to, const Mat4x4 *from)
{
	m_pParent = NULL;
	m_Props.m_ActorId = actorId;
	m_Props.m_Name = (renderComponent) ? renderComponent->VGetName() : "SceneNode";
	m_Props.m_RenderPass = renderPass;
	m_Props.m_AlphaType = AlphaOpaque;
	m_RenderComponent = renderComponent;
	VSetTransform(to, from);
	SetRadius(0);
}


SceneNode::~SceneNode()
{
}



HRESULT SceneNode::VOnRestore(Scene *pScene)
{
	Color color = (m_RenderComponent) ? m_RenderComponent->GetColor() : g_White;
	m_Props.m_Material.SetDiffuse(color);

	SceneNodeList::iterator i = m_Children.begin();
	SceneNodeList::iterator end = m_Children.end();
	while (i != end)
	{
		(*i)->VOnRestore(pScene);
		++i;
	}
	return S_OK;
}


HRESULT SceneNode::VOnLostDevice(Scene *pScene)
{

	SceneNodeList::iterator i = m_Children.begin();
	SceneNodeList::iterator end = m_Children.end();
	while (i != end)
	{
		(*i)->VOnLostDevice(pScene);
		++i;
	}
	return S_OK;
}


void SceneNode::VSetTransform(const Mat4x4 *toWorld, const Mat4x4 *fromWorld)
{
	m_Props.m_ToWorld = *toWorld;
	if (!fromWorld)
	{
		m_Props.m_FromWorld = m_Props.m_ToWorld.Inverse();
	}
	else
	{
		m_Props.m_FromWorld = *fromWorld;
	}
}


HRESULT SceneNode::VPreRender(Scene *pScene)
{
	StrongActorPtr pActor = MakeStrongPtr(g_pApp->GetGameLogic()->VGetActor(m_Props.m_ActorId));
	if (pActor)
	{
		shared_ptr<TransformComponent> pTc = MakeStrongPtr(pActor->GetComponent<TransformComponent>(TransformComponent::s_Name));
		if (pTc)
		{
			m_Props.m_ToWorld = pTc->GetTransform();
		}
	}

	pScene->PushAndSetMatrix(m_Props.m_ToWorld);
	return S_OK;
}



HRESULT SceneNode::VPostRender(Scene *pScene)
{
	pScene->PopMatrix();
	return S_OK;
}



bool SceneNode::VIsVisible(Scene *pScene) const
{
	// Transform the location of this node into the camera space 
	// of the camera attached to the scene

	Mat4x4 toWorld, fromWorld;
	pScene->GetCamera()->VGet()->Transform(&toWorld, &fromWorld);

	Vec3 pos = GetWorldPosition();

	Vec3 fromWorldPos = fromWorld.Xform(pos);

	Frustum const &frustum = pScene->GetCamera()->GetFrustum();

	bool isVisible = frustum.Inside(fromWorldPos, VGet()->Radius());
	return isVisible;
}

const Vec3 SceneNode::GetWorldPosition() const
{
	Vec3 pos = GetPosition();
	if (m_pParent)
	{
		pos += m_pParent->GetWorldPosition();
	}
	return pos;
}



HRESULT SceneNode::VOnUpdate(Scene *pScene, DWORD const elapsedMs)
{

	SceneNodeList::iterator i = m_Children.begin();
	SceneNodeList::iterator end = m_Children.end();

	while (i != end)
	{
		(*i)->VOnUpdate(pScene, elapsedMs);
		++i;
	}
	return S_OK;
}



HRESULT D3DSceneNode9::VRender(Scene *pScene)
{
	m_Props.GetMaterial().D3DUse9();

	switch (m_Props.AlphaType())
	{
	case AlphaTexture:
		break;

	case AlphaMaterial:
		DXUTGetD3D9Device()->SetRenderState(D3DRS_COLORVERTEX, true);
		DXUTGetD3D9Device()->SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_MATERIAL);
		break;

	case AlphaVertex:
		ASSERT(0 && _T("Not implemented!"));
		break;
	}

	return S_OK;
}



HRESULT SceneNode::VRenderChildren(Scene *pScene)
{
	// Iterate through the children....
	SceneNodeList::iterator i = m_Children.begin();
	SceneNodeList::iterator end = m_Children.end();

	while (i != end)
	{
		if ((*i)->VPreRender(pScene) == S_OK)
		{
			if ((*i)->VIsVisible(pScene))
			{
				float alpha = (*i)->VGet()->m_Material.GetAlpha();
				if (alpha == fOPAQUE)
				{
					(*i)->VRender(pScene);
				}
				else if (alpha != fTRANSPARENT)
				{
					// The object isn't totally transparent...
					AlphaSceneNode *asn = New AlphaSceneNode;
					ASSERT(asn);
					asn->m_pNode = *i;
					asn->m_Concat = pScene->GetTopMatrix();

					Vec4 worldPos(asn->m_Concat.GetPosition());

					Mat4x4 fromWorld = pScene->GetCamera()->VGet()->FromWorld();

					Vec4 screenPos = fromWorld.Xform(worldPos);

					asn->m_ZOrder = screenPos.z;

					pScene->AddAlphaSceneNode(asn);
				}

				(*i)->VRenderChildren(pScene);
			}
		}
		(*i)->VPostRender(pScene);
		++i;
	}

	return S_OK;
}



bool SceneNode::VAddChild(shared_ptr<ISceneNode> ikid)
{
	m_Children.push_back(ikid);

	shared_ptr<SceneNode> kid = static_pointer_cast<SceneNode>(ikid);

	kid->m_pParent = this;

	Vec3 kidPos = kid->VGet()->ToWorld().GetPosition();

	float newRadius = kidPos.Length() + kid->VGet()->Radius();

	if (newRadius > m_Props.m_Radius)
		m_Props.m_Radius = newRadius;

	return true;
}


bool SceneNode::VRemoveChild(ActorId id)
{
	for (SceneNodeList::iterator i = m_Children.begin(); i != m_Children.end(); ++i)
	{
		const SceneNodeProperties* pProps = (*i)->VGet();
		if (pProps->ActorId() != INVALID_ACTOR_ID && id == pProps->ActorId())
		{
			i = m_Children.erase(i);
			return true;
		}
	}
	return false;
}


HRESULT SceneNode::VPick(Scene *pScene, RayCast *raycast)
{
	for (SceneNodeList::const_iterator i = m_Children.begin(); i != m_Children.end(); ++i)
	{
		HRESULT hr = (*i)->VPick(pScene, raycast);

		if (hr == E_FAIL)
			return E_FAIL;
	}

	return S_OK;
}



void SceneNode::SetAlpha(float alpha)
{
	m_Props.SetAlpha(alpha);
	for (SceneNodeList::const_iterator i = m_Children.begin(); i != m_Children.end(); ++i)
	{
		shared_ptr<SceneNode> sceneNode = static_pointer_cast<SceneNode>(*i);
		sceneNode->SetAlpha(alpha);
	}
}


RootNode::RootNode()
	: SceneNode(INVALID_ACTOR_ID, WeakBaseRenderComponentPtr(), RENDERPASS_0, &Mat4x4::s_Identity)
{
	m_Children.reserve(RENDERPASS_LAST);

	shared_ptr<SceneNode> staticGroup(New SceneNode(INVALID_ACTOR_ID, WeakBaseRenderComponentPtr(), RENDERPASS_STATIC, &Mat4x4::s_Identity));
	m_Children.push_back(staticGroup);

	shared_ptr<SceneNode> actorGroup(New SceneNode(INVALID_ACTOR_ID, WeakBaseRenderComponentPtr(), RENDERPASS_ACTOR, &Mat4x4::s_Identity));
	m_Children.push_back(actorGroup);

	shared_ptr<SceneNode> skyGroup(New SceneNode(INVALID_ACTOR_ID, WeakBaseRenderComponentPtr(), RENDERPASS_SKY, &Mat4x4::s_Identity));
	m_Children.push_back(skyGroup);

	shared_ptr<SceneNode> invisibleGroup(New SceneNode(INVALID_ACTOR_ID, WeakBaseRenderComponentPtr(), RENDERPASS_NOT_RENDERED, &Mat4x4::s_Identity));
	m_Children.push_back(invisibleGroup);
}



bool RootNode::VAddChild(shared_ptr<ISceneNode> kid)
{
	// The Root node has children that divide the scene graph into render passes.
	// Scene nodes will get added to these children based on the value of the
	// render pass member variable.

	RenderPass pass = kid->VGet()->RenderPass();
	if ((unsigned)pass >= m_Children.size() || !m_Children[pass])
	{
		ASSERT(0 && _T("There is no such render pass"));
		return false;
	}

	return m_Children[pass]->VAddChild(kid);
}


bool RootNode::VRemoveChild(ActorId id)
{
	bool anythingRemoved = false;
	for (int i = RENDERPASS_0; i < RENDERPASS_LAST; ++i)
	{
		if (m_Children[i]->VRemoveChild(id))
		{
			anythingRemoved = true;
		}
	}
	return anythingRemoved;
}


HRESULT RootNode::VRenderChildren(Scene *pScene)
{
	// This control the render passes.

	for (int pass = RENDERPASS_0; pass < RENDERPASS_LAST; ++pass)
	{
		switch (pass)
		{
		case RENDERPASS_STATIC:
		case RENDERPASS_ACTOR:
			m_Children[pass]->VRenderChildren(pScene);
			break;

		case RENDERPASS_SKY:
		{
			shared_ptr<IRenderState> skyPass = pScene->GetRenderer()->VPrepareSkyBoxPass();
			m_Children[pass]->VRenderChildren(pScene);
			break;
		}
		}
	}

	return S_OK;
}



HRESULT CameraNode::VRender(Scene *pScene)
{
	if (m_DebugCamera)
	{
		pScene->PopMatrix();

		m_Frustum.Render();

		pScene->PushAndSetMatrix(m_Props.ToWorld());
	}

	return S_OK;
}



HRESULT CameraNode::VOnRestore(Scene *pScene)
{
	m_Frustum.SetAspect(DXUTGetWindowWidth() / (FLOAT)DXUTGetWindowHeight());
	D3DXMatrixPerspectiveFovLH(&m_Projection, m_Frustum.m_Fov, m_Frustum.m_Aspect, m_Frustum.m_Near, m_Frustum.m_Far);
	pScene->GetRenderer()->VSetProjectionTransform(&m_Projection);
	return S_OK;
}



HRESULT CameraNode::SetViewTransform(Scene *pScene)
{
	//Update camera transform
	if (m_pTarget)
	{
		Mat4x4 mat = m_pTarget->VGet()->ToWorld();
		Vec4 at = m_CamOffsetVector;
		Vec4 atWorld = mat.Xform(at);
		Vec3 pos = mat.GetPosition() + Vec3(atWorld);
		mat.SetPosition(pos);
		VSetTransform(&mat);
	}

	m_View = VGet()->FromWorld();

	pScene->GetRenderer()->VSetViewTransform(&m_View);
	return S_OK;
}



Mat4x4 CameraNode::GetWorldViewProjection(Scene *pScene)
{
	Mat4x4 world = pScene->GetTopMatrix();
	Mat4x4 view = VGet()->FromWorld();
	Mat4x4 worldView = world * view;
	return worldView * m_Projection;
}



D3DGrid9::D3DGrid9(ActorId actorId, WeakBaseRenderComponentPtr renderComponent, const Mat4x4* pMatrix)
	: SceneNode(actorId, renderComponent, RENDERPASS_0, pMatrix)
{
	m_TextureHasAlpha = false;
	m_pVerts = NULL;
	m_pIndices = NULL;
	m_NumVerts = m_NumPolys = 0;
}



D3DGrid9::~D3DGrid9()
{
	SAFE_RELEASE(m_pVerts);
	SAFE_RELEASE(m_pIndices);
}



HRESULT D3DGrid9::VOnRestore(Scene *pScene)
{
	SceneNode::VOnRestore(pScene);

	GridRenderComponent* grc = static_cast<GridRenderComponent*>(m_RenderComponent);

	int squares = grc->GetDivision();

	SAFE_RELEASE(m_pVerts);
	SAFE_RELEASE(m_pIndices);

	SetRadius(sqrt(squares * squares / 2.0f));

	// Create the vertex buffer - we'll need enough verts
	// to populate the grid.
	m_NumVerts = (squares + 1)*(squares + 1);

	if (FAILED(DXUTGetD3D9Device()->CreateVertexBuffer(
		m_NumVerts * sizeof(D3D9Vertex_ColoredTextured),
		D3DUSAGE_WRITEONLY, D3D9Vertex_ColoredTextured::FVF,
		D3DPOOL_MANAGED, &m_pVerts, NULL)))
	{
		return E_FAIL;
	}

	// Fill the vertex buffer. 
	D3D9Vertex_ColoredTextured* pVertices;
	if (FAILED(m_pVerts->Lock(0, 0, (void**)&pVertices, 0)))
		return E_FAIL;

	for (int j = 0; j < (squares + 1); j++)
	{
		for (int i = 0; i < (squares + 1); i++)
		{
			// Which vertex are we setting
			int index = i + (j * (squares + 1));
			D3D9Vertex_ColoredTextured *vert = &pVertices[index];

			// Default position of the grid is centered on the origin, flat on
			// the XZ plane.
			float x = (float)i - (squares / 2.0f);
			float y = (float)j - (squares / 2.0f);
			vert->position = (x * Vec3(1.f, 0.f, 0.f)) + (y * Vec3(0.f, 0.f, 1.f));
			vert->color = m_Props.GetMaterial().GetDiffuse();

			// The texture coordinates are set to x,y to make the
			// texture tile along with units - 1.0, 2.0, 3.0, etc.
			vert->tu = x;
			vert->tv = y;
		}
	}
	m_pVerts->Unlock();

	// The number of indicies equals the number of polygons times 3
	// since there are 3 indicies per polygon. Each grid square contains
	// two polygons.
	m_NumPolys = squares * squares * 2;
	if (FAILED(DXUTGetD3D9Device()->CreateIndexBuffer(sizeof(WORD) * m_NumPolys * 3,
		D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, D3DPOOL_MANAGED, &m_pIndices, NULL)))
	{
		return E_FAIL;
	}

	WORD *pIndices;
	if (FAILED(m_pIndices->Lock(0, 0, (void**)&pIndices, 0)))
		return E_FAIL;

	// Loop through the grid squares and calc the values
	// of each index. Each grid square has two triangles:
	//
	//		A - B
	//		| / |
	//		C - D

	for (int j = 0; j < squares; j++)
	{
		for (int i = 0; i < squares; i++)
		{
			// Triangle  ACB
			*(pIndices) = WORD(i + (j*(squares + 1)));
			*(pIndices + 1) = WORD(i + ((j + 1)*(squares + 1)));
			*(pIndices + 2) = WORD((i + 1) + (j*(squares + 1)));

			// Triangle  BCD
			*(pIndices + 3) = WORD((i + 1) + (j*(squares + 1)));
			*(pIndices + 4) = WORD(i + ((j + 1)*(squares + 1)));
			*(pIndices + 5) = WORD((i + 1) + ((j + 1)*(squares + 1)));
			pIndices += 6;
		}
	}

	m_pIndices->Unlock();

	return S_OK;
}



HRESULT D3DGrid9::VRender(Scene *pScene)
{
	DWORD oldLightMode;
	DXUTGetD3D9Device()->GetRenderState(D3DRS_LIGHTING, &oldLightMode);
	DXUTGetD3D9Device()->SetRenderState(D3DRS_LIGHTING, FALSE);

	DWORD oldCullMode;
	DXUTGetD3D9Device()->GetRenderState(D3DRS_CULLMODE, &oldCullMode);
	DXUTGetD3D9Device()->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

	// Setup our texture. Using textures introduces the texture stage states,
	// which govern how textures get blended together  In this case, we blending
	//our texture with the diffuse color of the vertices.

	GridRenderComponent* grc = static_cast<GridRenderComponent*>(m_RenderComponent);

	Resource resource(grc->GetTextureResource());
	shared_ptr<ResHandle> texture = g_pApp->m_ResCache->GetHandle(&resource);
	shared_ptr<D3DTextureResourceExtraData9> extra = static_pointer_cast<D3DTextureResourceExtraData9>(texture->GetExtra());
	DXUTGetD3D9Device()->SetTexture(0, extra->GetTexture());

	if (m_TextureHasAlpha)
	{
		DXUTGetD3D9Device()->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
		DXUTGetD3D9Device()->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);

		DXUTGetD3D9Device()->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);

		DXUTGetD3D9Device()->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
		DXUTGetD3D9Device()->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);

		DXUTGetD3D9Device()->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
	}
	else
	{
		DXUTGetD3D9Device()->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
		DXUTGetD3D9Device()->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
		DXUTGetD3D9Device()->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
	}

	DXUTGetD3D9Device()->SetStreamSource(0, m_pVerts, 0, sizeof(D3D9Vertex_ColoredTextured));
	DXUTGetD3D9Device()->SetIndices(m_pIndices);
	DXUTGetD3D9Device()->SetFVF(D3D9Vertex_ColoredTextured::FVF);
	DXUTGetD3D9Device()->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, m_NumVerts, 0, m_NumPolys);


	DXUTGetD3D9Device()->SetTexture(0, NULL);
	DXUTGetD3D9Device()->SetRenderState(D3DRS_LIGHTING, oldLightMode);
	DXUTGetD3D9Device()->SetRenderState(D3DRS_CULLMODE, oldCullMode);

	return S_OK;
}


HRESULT D3DGrid9::VPick(Scene *pScene, RayCast *pRayCast)
{
	if (SceneNode::VPick(pScene, pRayCast) == E_FAIL)
		return E_FAIL;

	pScene->PushAndSetMatrix(m_Props.ToWorld());

	HRESULT hr = pRayCast->Pick(pScene, m_Props.ActorId(), m_pVerts, m_pIndices, m_NumPolys);

	pScene->PopMatrix();

	return hr;
}



D3DGrid11::D3DGrid11(ActorId actorId, WeakBaseRenderComponentPtr renderComponent, const Mat4x4* pMatrix)
	: SceneNode(actorId, renderComponent, RENDERPASS_0, pMatrix)
{
	m_TextureHasAlpha = false;
	m_NumVerts = m_NumPolys = 0;
	m_pVertexBuffer = NULL;
	m_pIndexBuffer = NULL;
}


D3DGrid11::~D3DGrid11()
{
	SAFE_RELEASE(m_pVertexBuffer);
	SAFE_RELEASE(m_pIndexBuffer);
}


HRESULT D3DGrid11::VOnRestore(Scene *pScene)
{
	HRESULT hr;

	V_RETURN(SceneNode::VOnRestore(pScene));

	SAFE_RELEASE(m_pVertexBuffer);
	SAFE_RELEASE(m_pIndexBuffer);

	V_RETURN(m_VertexShader.OnRestore(pScene));
	V_RETURN(m_PixelShader.OnRestore(pScene));

	GridRenderComponent* grc = static_cast<GridRenderComponent*>(m_RenderComponent);

	int squares = grc->GetDivision();

	SetRadius(sqrt(squares * squares / 2.0f));

	// Create the vertex buffer - we'll need enough verts
	// to populate the grid.
	m_NumVerts = (squares + 1)*(squares + 1);

	// Fill the vertex buffer. We are setting the tu and tv texture
	// coordinates, which range from 0.0 to 1.0
	D3D11Vertex_UnlitTextured *pVerts = New D3D11Vertex_UnlitTextured[m_NumVerts];
	ASSERT(pVerts && "Out of memory in D3DGrid11::VOnRestore()");
	if (!pVerts)
		return E_FAIL;

	for (int j = 0; j < (squares + 1); j++)
	{
		for (int i = 0; i < (squares + 1); i++)
		{
			// Which vertex are we setting
			int index = i + (j * (squares + 1));
			D3D11Vertex_UnlitTextured *vert = &pVerts[index];

			// Default position of the grid is centered on the origin, flat on
			// the XZ plane.
			float x = (float)i - (squares / 2.0f);
			float y = (float)j - (squares / 2.0f);
			vert->Pos = Vec3(x, 0.f, y);
			vert->Normal = Vec3(0.0f, 1.0f, 0.0f);

			// The texture coordinates are set to x,y to make the
			// texture tile along with units - 1.0, 2.0, 3.0, etc.
			vert->Uv.x = x;
			vert->Uv.y = y;
		}
	}

	// The number of indicies equals the number of polygons times 3
	// since there are 3 indicies per polygon. Each grid square contains
	// two polygons. 

	m_NumPolys = squares * squares * 2;

	WORD *pIndices = New WORD[m_NumPolys * 3];

	ASSERT(pIndices && "Out of memory in D3DGrid11::VOnRestore()");
	if (!pIndices)
		return E_FAIL;

	// Loop through the grid squares and calc the values
	// of each index. Each grid square has two triangles:
	//
	//		A - B
	//		| / |
	//		C - D

	WORD *current = pIndices;
	for (int j = 0; j < squares; j++)
	{
		for (int i = 0; i < squares; i++)
		{
			// Triangle  ACB
			*(current) = WORD(i + (j*(squares + 1)));
			*(current + 1) = WORD(i + ((j + 1)*(squares + 1)));
			*(current + 2) = WORD((i + 1) + (j*(squares + 1)));

			// Triangle BCD
			*(current + 3) = WORD((i + 1) + (j*(squares + 1)));
			*(current + 4) = WORD(i + ((j + 1)*(squares + 1)));
			*(current + 5) = WORD((i + 1) + ((j + 1)*(squares + 1)));
			current += 6;
		}
	}

	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(D3D11Vertex_UnlitTextured) * m_NumVerts;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = pVerts;
	hr = DXUTGetD3D11Device()->CreateBuffer(&bd, &InitData, &m_pVertexBuffer);
	if (SUCCEEDED(hr))
	{
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.ByteWidth = sizeof(WORD) * m_NumPolys * 3;
		bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		bd.CPUAccessFlags = 0;
		InitData.pSysMem = pIndices;
		hr = DXUTGetD3D11Device()->CreateBuffer(&bd, &InitData, &m_pIndexBuffer);
	}

	SAFE_DELETE_ARRAY(pVerts);
	SAFE_DELETE_ARRAY(pIndices);

	return hr;
}



HRESULT D3DGrid11::VRender(Scene *pScene)
{
	HRESULT hr;

	GridRenderComponent* grc = static_cast<GridRenderComponent*>(m_RenderComponent);
	m_PixelShader.SetTexture(grc->GetTextureResource());

	V_RETURN(m_VertexShader.SetupRender(pScene, this));
	V_RETURN(m_PixelShader.SetupRender(pScene, this));

	// Set vertex buffer
	UINT stride = sizeof(D3D11Vertex_UnlitTextured);
	UINT offset = 0;
	DXUTGetD3D11DeviceContext()->IASetVertexBuffers(0, 1, &m_pVertexBuffer, &stride, &offset);

	// Set index buffer
	DXUTGetD3D11DeviceContext()->IASetIndexBuffer(m_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);

	// Set primitive topology
	DXUTGetD3D11DeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	DXUTGetD3D11DeviceContext()->DrawIndexed(m_NumPolys * 3, 0, 0);

	return S_OK;
}



ArrowNode::ArrowNode(std::string name, WeakBaseRenderComponentPtr renderComponent, const float length, const Mat4x4 *t, const Color &diffuseColor)
	: SceneNode(INVALID_ACTOR_ID, renderComponent, RENDERPASS_0, t)
{
	D3DXCreateCylinder(DXUTGetD3D9Device(), 0.1f * length, 0.0f, 0.3f * length, 16, 1, &m_cone, NULL);
	D3DXCreateCylinder(DXUTGetD3D9Device(), 0.05f * length, 0.05f * length, 0.7f * length, 16, 2, &m_shaft, NULL);

	m_coneTrans.BuildTranslation(0, 0, (0.7f + 0.15f) * length);
	m_shaftTrans.BuildTranslation(0, 0, (0.35f * length));

	// The anchor point is at 0,0,0 - so the radius must incorporate the whole length.
	SetRadius(length);
}



HRESULT ArrowNode::VRender(Scene *pScene)
{
	if (S_OK != SceneNode::VRender(pScene))
		return E_FAIL;

	pScene->PushAndSetMatrix(m_shaftTrans);

	m_shaft->DrawSubset(0);
	pScene->PopMatrix();

	pScene->PushAndSetMatrix(m_coneTrans);
	m_cone->DrawSubset(0);

	pScene->PopMatrix();
	return S_OK;
}



HRESULT ArrowNode::VPick(Scene *pScene, RayCast *pRayCast)
{
	if (SceneNode::VPick(pScene, pRayCast) == E_FAIL)
		return E_FAIL;

	pScene->PushAndSetMatrix(m_shaftTrans);
	HRESULT hr = pRayCast->Pick(pScene, m_Props.ActorId(), m_shaft);
	pScene->PopMatrix();

	if (hr == E_FAIL)
		return hr;

	pScene->PushAndSetMatrix(m_coneTrans);
	hr = pRayCast->Pick(pScene, m_Props.ActorId(), m_cone);
	pScene->PopMatrix();

	return hr;
}



TestObject::TestObject(std::string name)
	: SceneNode(0, WeakBaseRenderComponentPtr(), RENDERPASS_0, NULL)
{
	m_color = 0xffffffff;

	m_pVerts = NULL;
	m_NumVerts = m_NumPolys = 0;
}



TestObject::~TestObject()
{
	SAFE_RELEASE(m_pVerts);
}



Vec3 TestObject::g_CubeVerts[] =
{
	Vec3(0.5,0.5,-0.5),
	Vec3(-0.5,0.5,-0.5),
	Vec3(-0.5,0.5,0.5),
	Vec3(0.5,0.5,0.5),
	Vec3(0.5,-0.5,-0.5),
	Vec3(-0.5,-0.5,-0.5),
	Vec3(-0.5,-0.5,0.5),
	Vec3(0.5,-0.5,0.5)
};



Vec3 TestObject::g_SquashedCubeVerts[] =
{
	Vec3(0.5f,0.5f,-0.25f),
	Vec3(-0.5f,0.5f,-0.25f),
	Vec3(-0.5f,0.5f,0.5f),
	Vec3(0.75f,0.5f,0.5f),
	Vec3(0.75f,-0.5f,-0.5f),
	Vec3(-0.5f,-0.5f,-0.5f),
	Vec3(-0.5f,-0.3f,0.5f),
	Vec3(0.5f,-0.3f,0.5f)
};



WORD TestObject::g_TestObjectIndices[][3] =
{
	{ 0,1,2 },
	{ 0,2,3 },
	{ 0,4,5 },
	{ 0,5,1 },
	{ 1,5,6 },
	{ 1,6,2 },
	{ 2,6,7 },
	{ 2,7,3 },
	{ 3,7,4 },
	{ 3,4,0 },
	{ 4,7,6 },
	{ 4,6,5 }
};



HRESULT TestObject::VOnRestore(Scene *pScene)
{
	// Call the base class's restore
	SceneNode::VOnRestore(pScene);

	Vec3 center;
	Vec3 *verts = m_squashed ? g_SquashedCubeVerts : g_CubeVerts;
	float radius;
	HRESULT hr = D3DXComputeBoundingSphere(
		static_cast<D3DXVECTOR3*>(verts), 8,
		D3DXGetFVFVertexSize(D3DFVF_XYZ),
		&center, &radius);

	SetRadius(radius);

	// Create the vertex buffer - this object is essentailly 
	// a squashed cube, but since we want each face to be flat shaded
	// each face needs its own set of verts - because each vert has a normal
	// and thus can't have any vert shared by adjacent faces.
	m_NumPolys = 12;
	m_NumVerts = m_NumPolys * 3;

	SAFE_RELEASE(m_pVerts);
	if (FAILED(DXUTGetD3D9Device()->CreateVertexBuffer(
		m_NumVerts * sizeof(D3D9Vertex_UnlitColored),
		D3DUSAGE_WRITEONLY, D3D9Vertex_UnlitColored::FVF,
		D3DPOOL_MANAGED, &m_pVerts, NULL)))
	{
		return E_FAIL;
	}

	// Fill the vertex buffer. We are setting the tu and tv texture
	// coordinates, which range from 0.0 to 1.0
	D3D9Vertex_UnlitColored* pVertices;
	if (FAILED(m_pVerts->Lock(0, 0, (void**)&pVertices, 0)))
		return E_FAIL;

	static Color colors[6] = { g_White, g_Gray65, g_Cyan, g_Red, g_Green, g_Blue };

	for (DWORD face = 0; face < m_NumPolys; ++face)
	{
		D3D9Vertex_UnlitColored* v = &pVertices[face * 3];
		v->position = verts[g_TestObjectIndices[face][0]];
		v->diffuse = colors[face / 2];
		v->specular = colors[face / 2];
		(v + 1)->position = verts[g_TestObjectIndices[face][1]];
		(v + 1)->diffuse = colors[face / 2];
		(v + 1)->specular = colors[face / 2];
		(v + 2)->position = verts[g_TestObjectIndices[face][2]];
		(v + 2)->diffuse = colors[face / 2];
		(v + 2)->specular = colors[face / 2];

		Vec3 a = v->position - (v + 1)->position;
		Vec3 b = (v + 2)->position - (v + 1)->position;

		Vec3 cross = a.Cross(b);
		cross /= cross.Length();
		v->normal = cross;
		(v + 1)->normal = cross;
		(v + 2)->normal = cross;
	}

	m_pVerts->Unlock();

	return S_OK;
}



HRESULT TestObject::VRender(Scene *pScene)
{
	if (S_OK != SceneNode::VRender(pScene))
		return E_FAIL;

	DXUTGetD3D9Device()->SetRenderState(D3DRS_LIGHTING, TRUE);
	DXUTGetD3D9Device()->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
	DXUTGetD3D9Device()->SetTexture(0, NULL);
	DXUTGetD3D9Device()->SetRenderState(D3DRS_COLORVERTEX, TRUE);
	DXUTGetD3D9Device()->SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_COLOR1);

	// Setup our texture. Using textures introduces the texture stage states,
	// which govern how textures get blended together  In this case, we blending
	//our texture with the diffuse color of the vertices.

	DXUTGetD3D9Device()->SetStreamSource(0, m_pVerts, 0, sizeof(D3D9Vertex_UnlitColored));
	DXUTGetD3D9Device()->SetFVF(D3D9Vertex_UnlitColored::FVF);

	DXUTGetD3D9Device()->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 12);

	return S_OK;
}