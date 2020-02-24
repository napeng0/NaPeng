#pragma once
#include"GameCodeStd.h"
#include<DXUTgui.h>
#include"GameCode\interface.h"


struct ConstantBuffer_Matrices
{
	Mat4x4 m_WorldViewProj;
	Mat4x4 m_World;
};


struct ConstantBuffer_Material
{
	Vec4 m_vDiffuseObjectColor;
	Vec4 m_vAmbientObjectColor;
	BOOL m_bHasTexture;
	Vec3 m_vUnused;
};


#define MAXIMUM_LIGHTS_SUPPORTED (8)


struct ConstantBuffer_Lighting
{
	Vec4 m_vLightDiffuse[MAXIMUM_LIGHTS_SUPPORTED];
	Vec4 m_vLightDir[MAXIMUM_LIGHTS_SUPPORTED];
	Vec4 m_vLightAmbient;
	UINT m_nNumLights;
	Vec3 m_vUnused;
};


class D3DRenderer : public IRenderer
{
public:
	static CDXUTDialogResourceManager g_DialogResourceManager;
	static CDXUTTextHelper* g_pTextHelper;

	virtual HRESULT VOnRestore() { return S_OK; }
	virtual void VShutDown()  { SAFE_DELETE(g_pTextHelper); }
};



class D3DRenderer9 : public D3DRenderer
{
public:
	D3DRenderer9() { m_pFont = NULL; m_pTextSprite = NULL; }

	virtual void VSetBackgroundColor(BYTE bgA, BYTE bgR, BYTE bgG, BYTE bgB) { m_backgroundColor = D3DCOLOR_ARGB(bgA, bgR, bgG, bgB); }
	virtual bool VPreRender();
	virtual bool VPostRender();
	virtual HRESULT VOnRestore();
	virtual void VCalcLighting(Lights *lights, int maximumLights);

	virtual void VShutDown() { D3DRenderer::VShutDown(); SAFE_RELEASE(m_pFont); SAFE_RELEASE(m_pTextSprite); }

	virtual void VSetWorldTransform(const Mat4x4 *m) { DXUTGetD3D9Device()->SetTransform(D3DTS_WORLD, m); }
	virtual void VSetViewTransform(const Mat4x4 *m) { DXUTGetD3D9Device()->SetTransform(D3DTS_VIEW, m); }
	virtual void VSetProjectionTransform(const Mat4x4 *m) { DXUTGetD3D9Device()->SetTransform(D3DTS_PROJECTION, m); }

	virtual shared_ptr<IRenderState> VPrepareAlphaPass();
	virtual shared_ptr<IRenderState> VPrepareSkyBoxPass();

	virtual void VDrawLine(const Vec3& from, const Vec3& to, const Color& color);

protected:
	D3DCOLOR m_backgroundColor;		// The color that the view is cleared to each frame

	ID3DXFont*			        m_pFont;
	ID3DXSprite*				m_pTextSprite;
};



class D3DLineDrawer11
{
public:
	D3DLineDrawer11() { m_pVertexBuffer = NULL; }
	~D3DLineDrawer11() { SAFE_RELEASE(m_pVertexBuffer); }

	void DrawLine(const Vec3& from, const Vec3& to, const Color& color);
	HRESULT OnRestore();

protected:
	Vec3						m_Verts[2];
	DrawLineShader		m_LineDrawerShader;
	ID3D11Buffer*               m_pVertexBuffer;
};



class D3DRenderer11 : public D3DRenderer
{
public:
	D3DRenderer11() { m_pLineDrawer = NULL; }
	virtual void VShutDown() { D3DRenderer::VShutDown(); SAFE_DELETE(m_pLineDrawer); }

	virtual void VSetBackgroundColor(BYTE bgA, BYTE bgR, BYTE bgG, BYTE bgB)
	{
		m_backgroundColor[0] = float(bgA) / 255.0f;
		m_backgroundColor[1] = float(bgR) / 255.0f;
		m_backgroundColor[2] = float(bgG) / 255.0f;
		m_backgroundColor[3] = float(bgB) / 255.0f;
	}

	virtual bool VPreRender();
	virtual bool VPostRender();
	virtual HRESULT VOnRestore();
	virtual void VCalcLighting(Lights *lights, int maximumLights) { }

	// These three functions are done for each shader, not as a part of beginning the render - so they do nothing in D3D11.
	virtual void VSetWorldTransform(const Mat4x4 *m) {  }
	virtual void VSetViewTransform(const Mat4x4 *m) {  }
	virtual void VSetProjectionTransform(const Mat4x4 *m) {  }

	virtual void VDrawLine(const Vec3& from, const Vec3& to, const Color& color);

	virtual shared_ptr<IRenderState> VPrepareAlphaPass();
	virtual shared_ptr<IRenderState> VPrepareSkyBoxPass();

	HRESULT CompileShader(LPCSTR pSrcData, SIZE_T SrcDataLen, LPCSTR pFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut);
	HRESULT CompileShaderFromFile(WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut);

protected:
	float								m_backgroundColor[4];

	D3DLineDrawer11						*m_pLineDrawer;
};