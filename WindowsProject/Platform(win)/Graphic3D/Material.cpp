#include"GameCodeStd.h"
#include"GameCode\GameCode.h"
#include"ResourceCache\ResCache.h"
#include"SceneNode.h"

Material::Material()
{
	ZeroMemory(&m_Material, sizeof(D3DMATERIAL9));
	m_Material.Diffuse = g_White;
	m_Material.Ambient = Color(0.10f, 0.10f, 0.10f, 1.0f);
	m_Material.Specular = g_White;
	m_Material.Emissive = g_Black;
}

void Material::SetAmbient(const Color &color)
{
	m_Material.Ambient = color;
}

void Material::SetDiffuse(const Color &color)
{
	m_Material.Diffuse = color;
}

void Material::SetSpecular(const Color &color, const float power)
{
	m_Material.Specular = color;
	m_Material.Power = power;
}

void Material::SetEmissive(const Color &color)
{
	m_Material.Emissive = color;
}

void Material::SetAlpha(float alpha)
{
	m_Material.Diffuse.a = alpha;
}

void Material::D3DUse9()
{
	DXUTGetD3D9Device()->SetMaterial(&m_Material);
}



class DdsResourceLoader : public TextureResourceLoader
{
public:
	virtual std::string VGetPattern() { return "*.dds"; }
};

shared_ptr<IResourceLoader> CreateDDSResourceLoader()
{
	return shared_ptr<IResourceLoader>(New DdsResourceLoader());
}




class JpgResourceLoader : public TextureResourceLoader
{
public:
	virtual std::string VGetPattern() { return "*.jpg"; }
};


shared_ptr<IResourceLoader> CreateJPGResourceLoader()
{
	return shared_ptr<IResourceLoader>(New JpgResourceLoader());
}


D3DTextureResourceExtraData9::D3DTextureResourceExtraData9()
	: m_pTexture(NULL)
{
}


D3DTextureResourceExtraData11::D3DTextureResourceExtraData11()
	: m_pTexture(NULL), m_pSamplerLinear(NULL)
{
}


unsigned int TextureResourceLoader::VGetLoadedResourceSize(char *rawBuffer, unsigned int rawSize)
{
	//DirectX can allocate its own buffer
	return 0;
}

bool TextureResourceLoader::VLoadResource(char *rawBuffer, unsigned int rawSize, shared_ptr<ResHandle> handle)
{
	GameCodeApp::Renderer renderer = GameCodeApp::GetRendererImpl();
	if (renderer == GameCodeApp::RENDERER_D3D9)
	{
		shared_ptr<D3DTextureResourceExtraData9> extra = shared_ptr<D3DTextureResourceExtraData9>(New D3DTextureResourceExtraData9());

		if (FAILED(D3DXCreateTextureFromFileInMemory(DXUTGetD3D9Device(), rawBuffer, rawSize, &extra->m_pTexture)))
			return false;
		else
		{
			handle->SetExtra(shared_ptr<D3DTextureResourceExtraData9>(extra));
			return true;
		}
	}
	else if (renderer == GameCodeApp::RENDERER_D3D11)
	{
		shared_ptr<D3DTextureResourceExtraData11> extra = shared_ptr<D3DTextureResourceExtraData11>(New D3DTextureResourceExtraData11());

		// Load the Texture
		if (FAILED(D3DX11CreateShaderResourceViewFromMemory(DXUTGetD3D11Device(), rawBuffer, rawSize, NULL, NULL, &extra->m_pTexture, NULL)))
			return false;

		// Create the sample state
		D3D11_SAMPLER_DESC sampDesc;
		ZeroMemory(&sampDesc, sizeof(sampDesc));
		sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
		sampDesc.MinLOD = 0;
		sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
		if (FAILED(DXUTGetD3D11Device()->CreateSamplerState(&sampDesc, &extra->m_pSamplerLinear)))
			return false;

		handle->SetExtra(shared_ptr<D3DTextureResourceExtraData11>(extra));
		return true;
	}

	ASSERT(0 && "Unsupported Renderer in TextureResourceLoader::VLoadResource");
	return false;
}