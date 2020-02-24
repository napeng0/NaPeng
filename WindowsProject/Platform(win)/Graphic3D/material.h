#pragma once
#include"geometry.h"
#include"GameCodeStd.h"
#include"ResourceCache\ResCache.h"


class Material
{
	D3DMATERIAL9 m_Material;
public:
	Material();
	void SetAmbient(const Color& color);
	const Color GetAmbient() { return m_Material.Ambient; }
	void SetDiffuse(const Color& color);
	const Color GetDiffuse() { return m_Material.Diffuse; }
	void SetSpecular(const Color& color, const float power);
	void GetSpecular(Color& color, float power)
	{
		color = m_Material.Specular;
		power = m_Material.Power;
	}
	void SetEmissive(const Color& color);
	const Color GetEmissive() { return m_Material.Emissive; }
	void SetAlpha(const float alpha);
	bool HasAlpha() const { return GetAlpha() != fOPAQUE; }
	float GetAlpha() const { return m_Material.Diffuse.a; }
	void D3DUse9();
};


class D3DTextureResourceExtraData9 : public IResourceExtraData
{
	friend class TextureResourceLoader;

public:
	D3DTextureResourceExtraData9();
	virtual ~D3DTextureResourceExtraData9() { SAFE_RELEASE(m_pTexture); }
	virtual std::string VToString() { return "D3DTextureResourceExtraData9"; }

	LPDIRECT3DTEXTURE9 const GetTexture() { return m_pTexture; }

protected:
	LPDIRECT3DTEXTURE9		m_pTexture;
};


class D3DTextureResourceExtraData11 : public IResourceExtraData
{
	friend class TextureResourceLoader;

public:
	D3DTextureResourceExtraData11();
	virtual ~D3DTextureResourceExtraData11() { SAFE_RELEASE(m_pTexture); SAFE_RELEASE(m_pSamplerLinear); }
	virtual std::string VToString() { return "D3DTextureResourceExtraData11"; }

	ID3D11ShaderResourceView * const *GetTexture() { return &m_pTexture; }
	ID3D11SamplerState * const *GetSampler() { return &m_pSamplerLinear; }

protected:
	ID3D11ShaderResourceView *m_pTexture;
	ID3D11SamplerState* m_pSamplerLinear;
};


class TextureResourceLoader : public IResourceLoader
{
public:
	virtual bool VUseRawFile() { return false; }
	virtual bool VDiscardRawBufferAfterLoad() { return true; }
	virtual unsigned int VGetLoadedResourceSize(char *rawBuffer, unsigned int rawSize);
	virtual bool VLoadResource(char *rawBuffer, unsigned int rawSize, shared_ptr<ResHandle> handle);
};