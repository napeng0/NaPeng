#include"GameCodeStd.h"
#include"GameCode\GameCode.h"
#include"Renderer.h"
#include"geometry.h"
#include"SceneNode.h"
#include"Shaders.h"
#include"Sky.h"



SkyNode::SkyNode(const char *pTextureBaseName)
	: SceneNode(INVALID_ACTOR_ID, WeakBaseRenderComponentPtr(), RENDERPASS_SKY, &Mat4x4::s_Identity)
	, m_bActive(true)
{
	m_textureBaseName = pTextureBaseName;
}


HRESULT SkyNode::VPreRender(Scene *pScene)
{
	Vec3 cameraPos = m_camera->VGet()->ToWorld().GetPosition();
	Mat4x4 mat = m_Props.ToWorld();
	mat.SetPosition(cameraPos);
	VSetTransform(&mat);

	return SceneNode::VPreRender(pScene);
}




D3DSkyNode9::D3DSkyNode9(const char *pTextureBaseName)
	: SkyNode(pTextureBaseName)
{
	for (int i = 0; i < 5; ++i)
	{
		m_pTexture[i] = NULL;
	}
	m_pVerts = NULL;
}



D3DSkyNode9::~D3DSkyNode9()
{
	for (int i = 0; i < 5; ++i)
	{
		SAFE_RELEASE(m_pTexture[i]);
	}
	SAFE_RELEASE(m_pVerts);
}




HRESULT D3DSkyNode9::VOnRestore(Scene *pScene)
{

	// Call the base class's restore
	SceneNode::VOnRestore(pScene);

	m_camera = pScene->GetCamera();					

	m_NumVerts = 20;

	SAFE_RELEASE(m_pVerts);
	if (FAILED(DXUTGetD3D9Device()->CreateVertexBuffer(
		m_NumVerts * sizeof(D3D9Vertex_ColoredTextured),
		D3DUSAGE_WRITEONLY, D3D9Vertex_ColoredTextured::FVF,
		D3DPOOL_MANAGED, &m_pVerts, NULL)))
	{
		return E_FAIL;
	}

	// Fill the vertex buffer. We are setting the tu and tv texture
	// coordinates, which range from 0.0 to 1.0
	D3D9Vertex_ColoredTextured* pVertices;
	if (FAILED(m_pVerts->Lock(0, 0, (void**)&pVertices, 0)))
		return E_FAIL;

	// Loop through the grid squares and calc the values
	// of each index. Each grid square has two triangles:
	//
	//		A - B
	//		| / |
	//		C - D

	D3D9Vertex_ColoredTextured skyVerts[4];
	D3DCOLOR skyVertColor = 0xffffffff;
	float dim = 50.0f;

	skyVerts[0].position = Vec3(dim, dim, dim); skyVerts[0].color = skyVertColor; skyVerts[0].tu = 1; skyVerts[0].tv = 0;
	skyVerts[1].position = Vec3(-dim, dim, dim); skyVerts[1].color = skyVertColor; skyVerts[1].tu = 0; skyVerts[1].tv = 0;
	skyVerts[2].position = Vec3(dim, -dim, dim); skyVerts[2].color = skyVertColor; skyVerts[2].tu = 1; skyVerts[2].tv = 1;
	skyVerts[3].position = Vec3(-dim, -dim, dim); skyVerts[3].color = skyVertColor; skyVerts[3].tu = 0; skyVerts[3].tv = 1;

	Vec3 triangle[3];
	triangle[0] = Vec3(0.f, 0.f, 0.f);
	triangle[1] = Vec3(5.f, 0.f, 0.f);
	triangle[2] = Vec3(5.f, 5.f, 0.f);

	Vec3 edge1 = triangle[1] - triangle[0];
	Vec3 edge2 = triangle[2] - triangle[0];

	Vec3 normal;
	normal = edge1.Cross(edge2);
	normal.Normalize();

	Mat4x4 rotY;
	rotY.BuildRotationY(PI / 2.0f);
	Mat4x4 rotX;
	rotX.BuildRotationX(-PI / 2.0f);

	m_sides = 5;

	for (DWORD side = 0; side < m_sides; side++)
	{
		for (DWORD v = 0; v < 4; v++)
		{
			Vec4 temp;
			if (side < m_sides - 1)
			{
				temp = rotY.Xform(Vec3(skyVerts[v].position));
			}
			else
			{
				skyVerts[0].tu = 1; skyVerts[0].tv = 1;
				skyVerts[1].tu = 1; skyVerts[1].tv = 0;
				skyVerts[2].tu = 0; skyVerts[2].tv = 1;
				skyVerts[3].tu = 0; skyVerts[3].tv = 0;

				temp = rotX.Xform(Vec3(skyVerts[v].position));
			}
			skyVerts[v].position = Vec3(temp.x, temp.y, temp.z);
		}
		memcpy(&pVertices[side * 4], skyVerts, sizeof(skyVerts));
	}

	m_pVerts->Unlock();
	return S_OK;
}



HRESULT D3DSkyNode9::VRender(Scene *pScene)
{

	DXUTGetD3D9Device()->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
	DXUTGetD3D9Device()->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	DXUTGetD3D9Device()->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);

	DXUTGetD3D9Device()->SetStreamSource(0, m_pVerts, 0, sizeof(D3D9Vertex_ColoredTextured));
	DXUTGetD3D9Device()->SetFVF(D3D9Vertex_ColoredTextured::FVF);

	for (DWORD side = 0; side < m_sides; side++)
	{

		std::string name = GetTextureName(side);

		Resource resource(name);
		shared_ptr<ResHandle> texture = g_pApp->m_ResCache->GetHandle(&resource);
		shared_ptr<D3DTextureResourceExtraData9> extra = static_pointer_cast<D3DTextureResourceExtraData9>(texture->GetExtra());

		DXUTGetD3D9Device()->SetTexture(0, extra->GetTexture());
		DXUTGetD3D9Device()->DrawPrimitive(D3DPT_TRIANGLESTRIP, 4 * side, 2);
	}

	DXUTGetD3D9Device()->SetTexture(0, NULL);
	return S_OK;
}


D3DSkyNode11::D3DSkyNode11(const char *pTextureBaseName)
	: SkyNode(pTextureBaseName)
{
	m_pVertexBuffer = NULL;
	m_pIndexBuffer = NULL;
	m_VertexShader.EnableLights(false);
}



D3DSkyNode11::~D3DSkyNode11()
{
	SAFE_RELEASE(m_pVertexBuffer);
	SAFE_RELEASE(m_pIndexBuffer);
}





HRESULT D3DSkyNode11::VOnRestore(Scene *pScene)
{
	HRESULT hr;

	V_RETURN(SceneNode::VOnRestore(pScene));

	m_camera = pScene->GetCamera();

	SAFE_RELEASE(m_pVertexBuffer);
	SAFE_RELEASE(m_pIndexBuffer);

	V_RETURN(m_VertexShader.OnRestore(pScene));
	V_RETURN(m_PixelShader.OnRestore(pScene));

	m_NumVerts = 20;

	// Fill the vertex buffer. We are setting the tu and tv texture
	// coordinates, which range from 0.0 to 1.0
	D3D11Vertex_UnlitTextured *pVertices = New D3D11Vertex_UnlitTextured[m_NumVerts];
	ASSERT(pVertices && "Out of memory in D3DSkyNode11::VOnRestore()");
	if (!pVertices)
		return E_FAIL;

	// Loop through the grid squares and calc the values
	// of each index. Each grid square has two triangles:
	//
	//		A - B
	//		| / |
	//		C - D

	D3D11Vertex_UnlitTextured skyVerts[4];
	D3DCOLOR skyVertColor = 0xffffffff;
	float dim = 50.0f;

	skyVerts[0].Pos = Vec3(dim, dim, dim); skyVerts[0].Uv = Vec2(1.0f, 0.0f);
	skyVerts[1].Pos = Vec3(-dim, dim, dim); skyVerts[1].Uv = Vec2(0.0f, 0.0f);
	skyVerts[2].Pos = Vec3(dim, -dim, dim); skyVerts[2].Uv = Vec2(1.0f, 1.0f);
	skyVerts[3].Pos = Vec3(-dim, -dim, dim); skyVerts[3].Uv = Vec2(0.0f, 1.0f);

	Vec3 triangle[3];
	triangle[0] = Vec3(0.f, 0.f, 0.f);
	triangle[1] = Vec3(5.f, 0.f, 0.f);
	triangle[2] = Vec3(5.f, 5.f, 0.f);

	Vec3 edge1 = triangle[1] - triangle[0];
	Vec3 edge2 = triangle[2] - triangle[0];

	Vec3 normal;
	normal = edge1.Cross(edge2);
	normal.Normalize();

	Mat4x4 rotY;
	rotY.BuildRotationY(PI / 2.0f);
	Mat4x4 rotX;
	rotX.BuildRotationX(-PI / 2.0f);

	m_sides = 5;

	for (DWORD side = 0; side < m_sides; side++)
	{
		for (DWORD v = 0; v < 4; v++)
		{
			Vec4 temp;
			if (side < m_sides - 1)
			{
				temp = rotY.Xform(Vec3(skyVerts[v].Pos));
			}
			else
			{
				skyVerts[0].Uv = Vec2(1.0f, 1.0f);
				skyVerts[1].Uv = Vec2(1.0f, 1.0f);
				skyVerts[2].Uv = Vec2(1.0f, 1.0f);
				skyVerts[3].Uv = Vec2(1.0f, 1.0f);

				temp = rotX.Xform(Vec3(skyVerts[v].Pos));
			}
			skyVerts[v].Pos = Vec3(temp.x, temp.y, temp.z);
		}
		memcpy(&pVertices[side * 4], skyVerts, sizeof(skyVerts));
	}

	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(D3D11Vertex_UnlitTextured) * m_NumVerts;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = pVertices;
	hr = DXUTGetD3D11Device()->CreateBuffer(&bd, &InitData, &m_pVertexBuffer);
	SAFE_DELETE(pVertices);
	if (FAILED(hr))
		return hr;


	// Loop through the grid squares and calc the values
	// of each index. Each grid square has two triangles:
	//
	//		A - B
	//		| / |
	//		C - D

	WORD *pIndices = New WORD[m_sides * 2 * 3];

	WORD *current = pIndices;
	for (DWORD i = 0; i < m_sides; i++)
	{
		// Triangle  ACB
		*(current) = WORD(i * 4);
		*(current + 1) = WORD(i * 4 + 2);
		*(current + 2) = WORD(i * 4 + 1);

		// Triangle  BCD
		*(current + 3) = WORD(i * 4 + 1);
		*(current + 4) = WORD(i * 4 + 2);
		*(current + 5) = WORD(i * 4 + 3);
		current += 6;
	}

	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(WORD) * m_sides * 2 * 3;        
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = 0;
	InitData.pSysMem = pIndices;
	hr = DXUTGetD3D11Device()->CreateBuffer(&bd, &InitData, &m_pIndexBuffer);
	SAFE_DELETE_ARRAY(pIndices);
	if (FAILED(hr))
		return hr;


	return S_OK;
}


HRESULT D3DSkyNode11::VRender(Scene *pScene)
{
	HRESULT hr;

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

	for (DWORD side = 0; side < m_sides; side++)
	{
		std::string name = GetTextureName(side);
		m_PixelShader.SetTexture(name.c_str());

		DXUTGetD3D11DeviceContext()->DrawIndexed(6, side * 6, 0);
	}
	return S_OK;
}


std::string SkyNode::GetTextureName(const int side)
{
	std::string name = m_textureBaseName;
	const char *letters[] = { "n", "e", "s", "w", "u" };
	unsigned int index = name.find_first_of("_");
	ASSERT(index >= 0 && index < name.length() - 1);
	if (index >= 0 && index < name.length() - 1)
	{
		name[index + 1] = *letters[side];
	}
	return name;
}