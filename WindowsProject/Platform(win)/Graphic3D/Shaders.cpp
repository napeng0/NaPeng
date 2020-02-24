#include"GameCodeStd.h"
#include"GameCode\GameCode.h"
#include"Renderer.h"
#include"geometry.h"
#include"light.h"
#include"Mesh.h"
#include"SceneNode.h"
#include"ResourceCache\ResCache.h"
#include<xnamath.h>
#include<d3dx11effect.h>
#include"Shaders.h"


#pragma comment(lib, "effects11.lib")


VertexShader::VertexShader()
{
	m_pVertexLayout11 = NULL;
	m_pVertexShader = NULL;
	m_pcbVSMatrices = NULL;
	m_pcbVSLighting = NULL;
	m_pcbVSMaterial = NULL;
	m_enableLights = true;
}

VertexShader::~VertexShader()
{
	SAFE_RELEASE(m_pVertexLayout11);
	SAFE_RELEASE(m_pVertexShader);
	SAFE_RELEASE(m_pcbVSMatrices);
	SAFE_RELEASE(m_pcbVSLighting);
	SAFE_RELEASE(m_pcbVSMaterial);
}

HRESULT VertexShader::OnRestore(Scene *pScene)
{
	HRESULT hr = S_OK;

	SAFE_RELEASE(m_pVertexLayout11);
	SAFE_RELEASE(m_pVertexShader);
	SAFE_RELEASE(m_pcbVSMatrices);
	SAFE_RELEASE(m_pcbVSLighting);
	SAFE_RELEASE(m_pcbVSMaterial);

	shared_ptr<D3DRenderer11> d3dRenderer11 = static_pointer_cast<D3DRenderer11>(pScene->GetRenderer());

	
	ID3DBlob* pVertexShaderBuffer = NULL;

	std::string hlslFileName = "Effects\\GameCode4_VS.hlsl";
	Resource resource(hlslFileName.c_str());
	shared_ptr<ResHandle> pResourceHandle = g_pApp->m_ResCache->GetHandle(&resource);  // this actually loads the HLSL file from the zip file
	if (FAILED(d3dRenderer11->CompileShader(pResourceHandle->Buffer(), pResourceHandle->Size(), hlslFileName.c_str(), "GameCode4_VSMain", "vs_4_0_level_9_1", &pVertexShaderBuffer)))
	{
		SAFE_RELEASE(pVertexShaderBuffer);
		return hr;
	}

	if (FAILED(DXUTGetD3D11Device()->CreateVertexShader(pVertexShaderBuffer->GetBufferPointer(),
		pVertexShaderBuffer->GetBufferSize(), NULL, &m_pVertexShader)))
	{
		SAFE_RELEASE(pVertexShaderBuffer);
		return hr;
	}

	DXUT_SetDebugName(m_pVertexShader, "GameCode4_VSMain");

	if (SUCCEEDED(DXUTGetD3D11Device()->CreateInputLayout(D3D11VertexLayout_UnlitTextured, ARRAYSIZE(D3D11VertexLayout_UnlitTextured), pVertexShaderBuffer->GetBufferPointer(),
		pVertexShaderBuffer->GetBufferSize(), &m_pVertexLayout11)))
	{
		DXUT_SetDebugName(m_pVertexLayout11, "Primary");

		// Setup constant buffers
		D3D11_BUFFER_DESC Desc;
		Desc.Usage = D3D11_USAGE_DYNAMIC;
		Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		Desc.MiscFlags = 0;

		Desc.ByteWidth = sizeof(ConstantBuffer_Matrices);
		V_RETURN(DXUTGetD3D11Device()->CreateBuffer(&Desc, NULL, &m_pcbVSMatrices));
		DXUT_SetDebugName(m_pcbVSMatrices, "ConstantBuffer_Matrices");

		Desc.ByteWidth = sizeof(ConstantBuffer_Lighting);
		V_RETURN(DXUTGetD3D11Device()->CreateBuffer(&Desc, NULL, &m_pcbVSLighting));
		DXUT_SetDebugName(m_pcbPSLighting, "ConstantBuffer_Lighting");

		Desc.ByteWidth = sizeof(ConstantBuffer_Material);
		V_RETURN(DXUTGetD3D11Device()->CreateBuffer(&Desc, NULL, &m_pcbVSMaterial));
		DXUT_SetDebugName(m_pcbVSMaterial, "ConstantBuffer_Material");
	}

	SAFE_RELEASE(pVertexShaderBuffer);
	return S_OK;
}


HRESULT VertexShader::SetupRender(Scene *pScene, SceneNode *pNode)
{
	HRESULT hr = S_OK;

	// Set the vertex shader and the vertex layout
	DXUTGetD3D11DeviceContext()->VSSetShader(m_pVertexShader, NULL, 0);
	DXUTGetD3D11DeviceContext()->IASetInputLayout(m_pVertexLayout11);

	// Get the projection & view matrix from the camera class
	Mat4x4 mWorldViewProjection = pScene->GetCamera()->GetWorldViewProjection(pScene);
	Mat4x4 mWorld = pScene->GetTopMatrix();

	D3D11_MAPPED_SUBRESOURCE MappedResource;

	// Transform matrix
	V(DXUTGetD3D11DeviceContext()->Map(m_pcbVSMatrices, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource));

	ConstantBuffer_Matrices* pVSMatricies = (ConstantBuffer_Matrices*)MappedResource.pData;
	D3DXMatrixTranspose(&pVSMatricies->m_WorldViewProj, &mWorldViewProjection);
	D3DXMatrixTranspose(&pVSMatricies->m_World, &mWorld);

	DXUTGetD3D11DeviceContext()->Unmap(m_pcbVSMatrices, 0);

	// Lighting
	V(DXUTGetD3D11DeviceContext()->Map(m_pcbVSLighting, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource));
	ConstantBuffer_Lighting* pLighting = (ConstantBuffer_Lighting*)MappedResource.pData;

	if (m_enableLights)
	{
		pScene->GetLightManager()->CalcLighting(pLighting, pNode);
	}
	else
	{
		pLighting->m_nNumLights = 0;
		pLighting->m_vLightAmbient = Vec4(1.0f, 1.0f, 1.0f, 1.0f);
	}

	DXUTGetD3D11DeviceContext()->Unmap(m_pcbVSLighting, 0);

	// Material
	V(DXUTGetD3D11DeviceContext()->Map(m_pcbVSMaterial, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource));
	ConstantBuffer_Material* pPSMaterial = (ConstantBuffer_Material*)MappedResource.pData;

	Color color = pNode->VGet()->GetMaterial().GetDiffuse();
	pPSMaterial->m_vDiffuseObjectColor = Vec4(color.r, color.g, color.b, color.a);
	color = (m_enableLights) ? pNode->VGet()->GetMaterial().GetAmbient() : Color(1.0f, 1.0f, 1.0f, 1.0f);
	pPSMaterial->m_vAmbientObjectColor = Vec4(color.r, color.g, color.b, color.a);
	pPSMaterial->m_bHasTexture = false;

	DXUTGetD3D11DeviceContext()->Unmap(m_pcbVSMaterial, 0);

	DXUTGetD3D11DeviceContext()->VSSetConstantBuffers(0, 1, &m_pcbVSMatrices);
	DXUTGetD3D11DeviceContext()->VSSetConstantBuffers(1, 1, &m_pcbVSLighting);
	DXUTGetD3D11DeviceContext()->VSSetConstantBuffers(2, 1, &m_pcbVSMaterial);

	return S_OK;
}




PixelShader::PixelShader()
{
	m_pPixelShader = NULL;
	m_pcbPSMaterial = NULL;
}



PixelShader::~PixelShader()
{
	SAFE_RELEASE(m_pPixelShader);
	SAFE_RELEASE(m_pcbPSMaterial);
}




HRESULT PixelShader::OnRestore(Scene *pScene)
{
	HRESULT hr = S_OK;

	SAFE_RELEASE(m_pPixelShader);
	SAFE_RELEASE(m_pcbPSMaterial);

	shared_ptr<D3DRenderer11> d3dRenderer11 = static_pointer_cast<D3DRenderer11>(pScene->GetRenderer());

	// Set up the pixel shader and related constant buffers

	// Compile the pixel shader using the lowest possible profile for broadest feature level support
	ID3DBlob* pPixelShaderBuffer = NULL;

	std::string hlslFileName = "Effects\\GameCode4_PS.hlsl";
	Resource resource(hlslFileName.c_str());
	shared_ptr<ResHandle> pResourceHandle = g_pApp->m_ResCache->GetHandle(&resource); 
	if (FAILED(d3dRenderer11->CompileShader(pResourceHandle->Buffer(), pResourceHandle->Size(), hlslFileName.c_str(), "GameCode4_PSMain", "ps_4_0_level_9_1", &pPixelShaderBuffer)))
	{
		SAFE_RELEASE(pPixelShaderBuffer);
		return hr;
	}

	if (SUCCEEDED(DXUTGetD3D11Device()->CreatePixelShader(pPixelShaderBuffer->GetBufferPointer(),
		pPixelShaderBuffer->GetBufferSize(), NULL, &m_pPixelShader)))
	{
		DXUT_SetDebugName(m_pPixelShader, "GameCode4_PSMain");

		// Setup constant buffers
		D3D11_BUFFER_DESC Desc;
		Desc.Usage = D3D11_USAGE_DYNAMIC;
		Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		Desc.MiscFlags = 0;

		Desc.ByteWidth = sizeof(ConstantBuffer_Material);
		hr = DXUTGetD3D11Device()->CreateBuffer(&Desc, NULL, &m_pcbPSMaterial);
		DXUT_SetDebugName(m_pcbPSMaterial, "ConstantBuffer_Material");
	}

	SAFE_RELEASE(pPixelShaderBuffer);
	return hr;
}




HRESULT PixelShader::SetupRender(Scene *pScene, SceneNode *pNode)
{
	HRESULT hr = S_OK;

	DXUTGetD3D11DeviceContext()->PSSetShader(m_pPixelShader, NULL, 0);

	D3D11_MAPPED_SUBRESOURCE MappedResource;

	V(DXUTGetD3D11DeviceContext()->Map(m_pcbPSMaterial, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource));
	ConstantBuffer_Material* pPSMaterial = (ConstantBuffer_Material*)MappedResource.pData;

	Color color = pNode->VGet()->GetMaterial().GetDiffuse();
	pPSMaterial->m_vDiffuseObjectColor = Vec4(color.r, color.g, color.b, color.a);

	if (m_textureResource.length() > 0)
	{
		pPSMaterial->m_bHasTexture = true;
	}
	else
	{
		pPSMaterial->m_bHasTexture = false;
	}

	DXUTGetD3D11DeviceContext()->Unmap(m_pcbPSMaterial, 0);

	DXUTGetD3D11DeviceContext()->PSSetConstantBuffers(0, 1, &m_pcbPSMaterial);

	// Set up the texture
	SetTexture(m_textureResource);

	return S_OK;
}




HRESULT PixelShader::SetTexture(const std::string& textureName)
{
	m_textureResource = textureName;
	if (m_textureResource.length() > 0)
	{
		Resource resource(m_textureResource);
		shared_ptr<ResHandle> texture = g_pApp->m_ResCache->GetHandle(&resource);
		if (texture)
		{
			shared_ptr<D3DTextureResourceExtraData11> extra = static_pointer_cast<D3DTextureResourceExtraData11>(texture->GetExtra());
			SetTexture(extra->GetTexture(), extra->GetSampler());
		}
	}
	return S_OK;
}

HRESULT PixelShader::SetTexture(ID3D11ShaderResourceView* const *pDiffuseRV, ID3D11SamplerState * const *ppSamplers)
{
	DXUTGetD3D11DeviceContext()->PSSetShaderResources(0, 1, pDiffuseRV);
	DXUTGetD3D11DeviceContext()->PSSetSamplers(0, 1, ppSamplers);
	return S_OK;
}



struct LineDrawerChangePerFrameBuffer
{
	Mat4x4 m_WorldViewProjection;
	Vec4 m_DiffuseColor;
};

DrawLineShader::DrawLineShader()
{
	m_pEffect = NULL;
	m_pVertexLayout11 = NULL;
	m_pcbChangePerFrame = NULL;
	m_pcbRenderTargetSize = NULL;
	m_pcbDiffuseColor = NULL;
}


DrawLineShader::~DrawLineShader()
{
	SAFE_RELEASE(m_pVertexLayout11);
	SAFE_RELEASE(m_pcbChangePerFrame);
	SAFE_RELEASE(m_pcbRenderTargetSize);
	SAFE_RELEASE(m_pcbDiffuseColor);
	SAFE_RELEASE(m_pEffect);
}




HRESULT DrawLineShader::OnRestore(Scene *pScene)
{
	HRESULT hr = S_OK;

	SAFE_RELEASE(m_pEffect);
	SAFE_RELEASE(m_pcbDiffuseColor);
	SAFE_RELEASE(m_pVertexLayout11);
	SAFE_RELEASE(m_pcbChangePerFrame);
	SAFE_RELEASE(m_pcbRenderTargetSize);

	shared_ptr<D3DRenderer11> d3dRenderer11 = static_pointer_cast<D3DRenderer11>(pScene->GetRenderer());


	// Set up the vertex shader and related constant buffers

	// Compile the vertex shader using the lowest possible profile for broadest feature level support
	ID3DBlob* pVertexShaderBuffer = NULL;

	std::string hlslFileName = "Effects\\LineDraw.hlsl";
	Resource resource(hlslFileName.c_str());
	shared_ptr<ResHandle> pResourceHandle = g_pApp->m_ResCache->GetHandle(&resource); 

	// Compile effect from HLSL file into binary Blob in memory
	// The code in this function was found here - http://asawicki.info/news_1371_effects_in_directx_11.html

	ID3D10Blob *effectBlob = 0, *errorsBlob = 0;
	hr = D3DX11CompileFromMemory(
		pResourceHandle->Buffer(),			// SrcData
		pResourceHandle->Size(),				// SrcLen
		0, 0, 0, 0,							// FileName, pDefines, pInclude, FunctionName
		"fx_5_0", 0, 0, 0,					// Profile, Flags1, Flags2, Pump
		&effectBlob, &errorsBlob, 0);			// Shader, ErrorMsg, pResult
	assert(SUCCEEDED(hr) && effectBlob);
	if (errorsBlob) errorsBlob->Release();

	// Create D3DX11 effect from compiled binary memory block
	if (FAILED(D3DX11CreateEffectFromMemory(
		effectBlob->GetBufferPointer(), effectBlob->GetBufferSize(), 0, DXUTGetD3D11Device(), &m_pEffect)))
	{
		return hr;
	}

	effectBlob->Release();

	m_EffectTechnique = m_pEffect->GetTechniqueByIndex(0);
	ASSERT(m_EffectTechnique && m_EffectTechnique->IsValid());

	m_EffectPass = m_EffectTechnique->GetPassByIndex(0);
	ASSERT(m_EffectPass && m_EffectPass->IsValid());

	D3DX11_PASS_SHADER_DESC effectVsDesc;
	m_EffectPass->GetVertexShaderDesc(&effectVsDesc);
	D3DX11_EFFECT_SHADER_DESC effectVsDesc2;
	effectVsDesc.pShaderVariable->GetShaderDesc(effectVsDesc.ShaderIndex, &effectVsDesc2);
	const void *vsCodePtr = effectVsDesc2.pBytecode;
	unsigned vsCodeLen = effectVsDesc2.BytecodeLength;

	if (SUCCEEDED(DXUTGetD3D11Device()->CreateInputLayout(D3D11VertexLayout_Position, ARRAYSIZE(D3D11VertexLayout_Position), vsCodePtr, vsCodeLen, &m_pVertexLayout11)))
	{
		DXUT_SetDebugName(m_pVertexLayout11, "Primary");

		// Setup constant buffers
		D3D11_BUFFER_DESC Desc;
		Desc.Usage = D3D11_USAGE_DYNAMIC;
		Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		Desc.MiscFlags = 0;

		Desc.ByteWidth = sizeof(Vec4);
		V_RETURN(DXUTGetD3D11Device()->CreateBuffer(&Desc, NULL, &m_pcbRenderTargetSize));
		DXUT_SetDebugName(m_pcbRenderTargetSize, "Vec4_RenderTargetSize");

		Desc.ByteWidth = sizeof(Mat4x4);
		V_RETURN(DXUTGetD3D11Device()->CreateBuffer(&Desc, NULL, &m_pcbChangePerFrame));
		DXUT_SetDebugName(m_pcbChangePerFrame, "LineDrawerChangePerFrameBuffer");

		Desc.ByteWidth = sizeof(Vec4);
		V_RETURN(DXUTGetD3D11Device()->CreateBuffer(&Desc, NULL, &m_pcbDiffuseColor));
		DXUT_SetDebugName(m_pcbDiffuseColor, "DiffuseColor");
	}

	SAFE_RELEASE(pVertexShaderBuffer);

	return S_OK;
}


HRESULT DrawLineShader::SetupRender(Scene *pScene)
{
	HRESULT hr = S_OK;

	m_EffectPass->Apply(0, DXUTGetD3D11DeviceContext());

	DXUTGetD3D11DeviceContext()->IASetInputLayout(m_pVertexLayout11);

	D3D11_MAPPED_SUBRESOURCE MappedResource;


	// Render Target size
	V(DXUTGetD3D11DeviceContext()->Map(m_pcbRenderTargetSize, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource));

	Vec4 * pRenderTargetSize = (Vec4 *)MappedResource.pData;
	pRenderTargetSize->x = (FLOAT)DXUTGetWindowWidth();
	pRenderTargetSize->y = (FLOAT)DXUTGetWindowHeight();
	pRenderTargetSize->z = 0.0f;
	pRenderTargetSize->w = 0.0f;

	DXUTGetD3D11DeviceContext()->Unmap(m_pcbRenderTargetSize, 0);

	// Transform matrix
	V(DXUTGetD3D11DeviceContext()->Map(m_pcbChangePerFrame, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource));

	LineDrawerChangePerFrameBuffer * pChangePerFrame = (LineDrawerChangePerFrameBuffer*)MappedResource.pData;
	Mat4x4 mWorldViewProjection = pScene->GetCamera()->GetWorldViewProjection(pScene);
	D3DXMatrixTranspose(&pChangePerFrame->m_WorldViewProjection, &mWorldViewProjection);

	DXUTGetD3D11DeviceContext()->Unmap(m_pcbChangePerFrame, 0);

	// Buffers

	ID3DX11EffectConstantBuffer* fxRenderTargetCB = m_pEffect->GetConstantBufferByName("cbRenderTarget")->AsConstantBuffer();
	ID3DX11EffectConstantBuffer* fxChangePerFrameCB = m_pEffect->GetConstantBufferByName("cbChangePerFrame")->AsConstantBuffer();
	ID3DX11EffectConstantBuffer* fxDiffuseCB = m_pEffect->GetConstantBufferByName("cbDiffuseColor")->AsConstantBuffer();

	fxRenderTargetCB->SetConstantBuffer(m_pcbRenderTargetSize);
	fxChangePerFrameCB->SetConstantBuffer(m_pcbChangePerFrame);
	fxDiffuseCB->SetConstantBuffer(m_pcbDiffuseColor);

	return S_OK;
}





HRESULT DrawLineShader::SetDiffuse(const std::string& textureName, const Color &color)
{
	m_textureResource = textureName;
	if (m_textureResource.length() > 0)
	{
		Resource resource(m_textureResource);
		shared_ptr<ResHandle> texture = g_pApp->m_ResCache->GetHandle(&resource);
		if (texture)
		{
			shared_ptr<D3DTextureResourceExtraData11> extra = static_pointer_cast<D3DTextureResourceExtraData11>(texture->GetExtra());
			SetTexture(extra->GetTexture(), extra->GetSampler());
		}
	}

	HRESULT hr;
	D3D11_MAPPED_SUBRESOURCE MappedResource;

	// Diffuse color
	V(DXUTGetD3D11DeviceContext()->Map(m_pcbDiffuseColor, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource));
	Vec4 * pDiffuseColor = (Vec4*)MappedResource.pData;
	*pDiffuseColor = Vec4(color.r, color.g, color.b, color.a);
	DXUTGetD3D11DeviceContext()->Unmap(m_pcbDiffuseColor, 0);

	return S_OK;
}

HRESULT DrawLineShader::SetTexture(ID3D11ShaderResourceView* const *pDiffuseRV, ID3D11SamplerState * const *ppSamplers)
{
	DXUTGetD3D11DeviceContext()->PSSetShaderResources(0, 1, pDiffuseRV);
	DXUTGetD3D11DeviceContext()->PSSetSamplers(0, 1, ppSamplers);
	return S_OK;
}