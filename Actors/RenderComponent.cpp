#include"GameCodeStd.h"
#include"Graphic3D\Mesh.h"
#include"Graphic3D\Sky.h"
#include"Utilities\String.h"
#include"EventManager\Events.h"

#include"Actors\RenderComponent.h"
#include"Actors\TransformComponent.h"



const char* MeshRenderComponent::s_Name = "MeshRenderComponent";
const char* SphereRenderComponent::s_Name = "SphereRenderComponent";
const char* TeapotRenderComponent::s_Name = "TeapotRenderComponent";
const char* GridRenderComponent::s_Name = "GridRenderComponent";
const char* LightRenderComponent::s_Name = "LightRenderComponent";
const char* SkyRenderComponent::s_Name = "SkyRenderComponent";


bool BaseRenderComponent::VInit(TiXmlElement* pData)
{
	// Color
	TiXmlElement* pColorNode = pData->FirstChildElement("Color");
	if (pColorNode)
		m_color = LoadColor(pColorNode);

	return VDelegateInit(pData);
}

void BaseRenderComponent::VPostInit(void)
{
	shared_ptr<SceneNode> pSceneNode(VGetSceneNode());
	shared_ptr<EvtData_New_Render_Component> pEvent(New EvtData_New_Render_Component(m_pOwner->GetActorId(), pSceneNode));
	IEventManager::Get()->VTriggerEvent(pEvent);
}


void BaseRenderComponent::VOnChanged(void)
{
	shared_ptr<EvtData_Modified_Render_Component> pEvent(New EvtData_Modified_Render_Component(m_pOwner->GetActorId()));
	IEventManager::Get()->VTriggerEvent(pEvent);
}


TiXmlElement* BaseRenderComponent::VGenerateXml(void)
{
	TiXmlElement* pBaseElement = VCreateBaseElement();

	// Color
	TiXmlElement* pColor = New TiXmlElement("Color");
	pColor->SetAttribute("r", ToStr(m_color.r).c_str());
	pColor->SetAttribute("g", ToStr(m_color.g).c_str());
	pColor->SetAttribute("b", ToStr(m_color.b).c_str());
	pColor->SetAttribute("a", ToStr(m_color.a).c_str());
	pBaseElement->LinkEndChild(pColor);

	// Create XML for inherited classes
	VCreateInheritedXmlElements(pBaseElement);

	return pBaseElement;
}

shared_ptr<SceneNode> BaseRenderComponent::VGetSceneNode(void)
{
	if (!m_pSceneNode)
		m_pSceneNode = VCreateSceneNode();
	return m_pSceneNode;
}

Color BaseRenderComponent::LoadColor(TiXmlElement* pData)
{
	Color color;

	double r = 1.0;
	double g = 1.0;
	double b = 1.0;
	double a = 1.0;

	pData->Attribute("r", &r);
	pData->Attribute("g", &g);
	pData->Attribute("b", &b);
	pData->Attribute("a", &a);

	color.r = (float)r;
	color.g = (float)g;
	color.b = (float)b;
	color.a = (float)a;

	return color;
}


shared_ptr<SceneNode> MeshRenderComponent::VCreateSceneNode(void)
{
	return shared_ptr<SceneNode>();
}

void MeshRenderComponent::VCreateInheritedXmlElements(TiXmlElement *)
{
	ERROR("MeshRenderComponent::VGenerateSubclassXml() not implemented");
}




SphereRenderComponent::SphereRenderComponent(void)
{
	m_segments = 50;
}

bool SphereRenderComponent::VDelegateInit(TiXmlElement* pData)
{
	TiXmlElement* pMesh = pData->FirstChildElement("Sphere");
	int segments = 50;
	double radius = 1.0;
	pMesh->Attribute("radius", &radius);
	pMesh->Attribute("segments", &segments);
	m_radius = (float)radius;
	m_segments = (unsigned int)segments;

	return true;
}

shared_ptr<SceneNode> SphereRenderComponent::VCreateSceneNode(void)
{
	// Get the transform component
	shared_ptr<TransformComponent> pTransformComponent = MakeStrongPtr(m_pOwner->GetComponent<TransformComponent>(TransformComponent::s_Name));
	if (!pTransformComponent)
	{
		// Can't render without a transform
		return shared_ptr<SceneNode>();
	}

	WeakBaseRenderComponentPtr wbrcp(this);
	if (GameCodeApp::GetRendererImpl() == GameCodeApp::RENDERER_D3D9)
	{
		// Create the sphere Mesh
		ID3DXMesh* pSphereMesh;

		D3DXCreateSphere(DXUTGetD3D9Device(), m_radius, m_segments, m_segments, &pSphereMesh, NULL);

		shared_ptr<SceneNode> sphere(New D3DShaderMeshNode9(m_pOwner->GetActorId(), wbrcp, pSphereMesh, "Effects\\GameCode4.fx", RENDERPASS_ACTOR, &pTransformComponent->GetTransform()));

		SAFE_RELEASE(pSphereMesh);
		return sphere;
	}
	else if (GameCodeApp::GetRendererImpl() == GameCodeApp::RENDERER_D3D11)
	{
		shared_ptr<SceneNode> sphere(New D3DShaderMeshNode11(m_pOwner->GetActorId(), wbrcp, "art\\sphere.sdkmesh", RENDERPASS_ACTOR, &pTransformComponent->GetTransform()));
		return sphere;
	}
	else
	{
		ASSERT(0 && "Unknown Renderer Implementation in SphereRenderComponent::VCreateSceneNode");
		return shared_ptr<SceneNode>(NULL);
	}
}

void SphereRenderComponent::VCreateInheritedXmlElements(TiXmlElement* pBaseElement)
{
	TiXmlElement* pMesh = New TiXmlElement("Sphere");
	pMesh->SetAttribute("radius", ToStr(m_radius).c_str());
	pMesh->SetAttribute("segments", ToStr(m_segments).c_str());
	pBaseElement->LinkEndChild(pBaseElement);
}



shared_ptr<SceneNode> TeapotRenderComponent::VCreateSceneNode(void)
{
	// Get the transform component
	shared_ptr<TransformComponent> pTransformComponent = MakeStrongPtr(m_pOwner->GetComponent<TransformComponent>(TransformComponent::s_Name));
	if (pTransformComponent)
	{
		WeakBaseRenderComponentPtr weakThis(this);

		switch (GameCodeApp::GetRendererImpl())
		{
		case GameCodeApp::RENDERER_D3D9:
			return shared_ptr<SceneNode>(New D3DTeapotMeshNode9(m_pOwner->GetActorId(), weakThis, "Effects\\GameCode4.fx", RENDERPASS_ACTOR, &pTransformComponent->GetTransform()));

		case GameCodeApp::RENDERER_D3D11:
		{
			Mat4x4 rot90;
			rot90.BuildRotationY(-PI / 2.0f);
			shared_ptr<SceneNode> parent(New SceneNode(m_pOwner->GetActorId(), weakThis, RENDERPASS_ACTOR, &pTransformComponent->GetTransform()));
			shared_ptr<SceneNode> teapot(New D3DTeapotMeshNode11(INVALID_ACTOR_ID, weakThis, RENDERPASS_ACTOR, &rot90));
			parent->VAddChild(teapot);
			return parent;
		}

		default:
			ERROR("Unknown Renderer Implementation in TeapotRenderComponent");
		}
	}

	return shared_ptr<SceneNode>();
}

void TeapotRenderComponent::VCreateInheritedXmlElements(TiXmlElement *)
{
}



GridRenderComponent::GridRenderComponent(void)
{
	m_textureResource = "";
	m_squares = 0;
}

bool GridRenderComponent::VDelegateInit(TiXmlElement* pData)
{
	TiXmlElement* pTexture = pData->FirstChildElement("Texture");
	if (pTexture)
	{
		m_textureResource = pTexture->FirstChild()->Value();
	}

	TiXmlElement* pDivision = pData->FirstChildElement("Division");
	if (pDivision)
	{
		m_squares = atoi(pDivision->FirstChild()->Value());
	}

	return true;
}

shared_ptr<SceneNode> GridRenderComponent::VCreateSceneNode(void)
{
	shared_ptr<TransformComponent> pTransformComponent = MakeStrongPtr(m_pOwner->GetComponent<TransformComponent>(TransformComponent::s_Name));
	if (pTransformComponent)
	{
		WeakBaseRenderComponentPtr weakThis(this);

		switch (GameCodeApp::GetRendererImpl())
		{
		case GameCodeApp::RENDERER_D3D9:
			return shared_ptr<SceneNode>(New D3DGrid9(m_pOwner->GetActorId(), weakThis, &(pTransformComponent->GetTransform())));

		case GameCodeApp::RENDERER_D3D11:
			return shared_ptr<SceneNode>(New D3DGrid11(m_pOwner->GetActorId(), weakThis, &(pTransformComponent->GetTransform())));

		default:
			ERROR("Unknown Renderer Implementation in GridRenderComponent");
		}
	}

	return shared_ptr<SceneNode>();
}

void GridRenderComponent::VCreateInheritedXmlElements(TiXmlElement *pBaseElement)
{
	TiXmlElement* pTextureNode = New TiXmlElement("Texture");
	TiXmlText* pTextureText = New TiXmlText(m_textureResource.c_str());
	pTextureNode->LinkEndChild(pTextureText);
	pBaseElement->LinkEndChild(pTextureNode);

	TiXmlElement* pDivisionNode = New TiXmlElement("Division");
	TiXmlText* pDivisionText = New TiXmlText(ToStr(m_squares).c_str());
	pDivisionNode->LinkEndChild(pDivisionText);
	pBaseElement->LinkEndChild(pDivisionNode);
}



LightRenderComponent::LightRenderComponent(void)
{
}

bool LightRenderComponent::VDelegateInit(TiXmlElement* pData)
{
	TiXmlElement* pLight = pData->FirstChildElement("Light");

	double temp;
	TiXmlElement* pAttenuationNode = NULL;
	pAttenuationNode = pLight->FirstChildElement("Attenuation");
	if (pAttenuationNode)
	{
		double temp;
		pAttenuationNode->Attribute("const", &temp);
		m_Props.m_Attenuation[0] = (float)temp;

		pAttenuationNode->Attribute("linear", &temp);
		m_Props.m_Attenuation[1] = (float)temp;

		pAttenuationNode->Attribute("exp", &temp);
		m_Props.m_Attenuation[2] = (float)temp;
	}

	TiXmlElement* pShapeNode = NULL;
	pShapeNode = pLight->FirstChildElement("Shape");
	if (pShapeNode)
	{
		pShapeNode->Attribute("range", &temp);
		m_Props.m_Range = (float)temp;
		pShapeNode->Attribute("falloff", &temp);
		m_Props.m_Falloff = (float)temp;
		pShapeNode->Attribute("theta", &temp);
		m_Props.m_Theta = (float)temp;
		pShapeNode->Attribute("phi", &temp);
		m_Props.m_Phi = (float)temp;
	}
	return true;
}

shared_ptr<SceneNode> LightRenderComponent::VCreateSceneNode(void)
{
	shared_ptr<TransformComponent> pTransformComponent = MakeStrongPtr(m_pOwner->GetComponent<TransformComponent>(TransformComponent::s_Name));
	if (pTransformComponent)
	{
		WeakBaseRenderComponentPtr weakThis(this);

		switch (GameCodeApp::GetRendererImpl())
		{
		case GameCodeApp::RENDERER_D3D9:
			return shared_ptr<SceneNode>(New D3DLightNode9(m_pOwner->GetActorId(), weakThis, m_Props, &(pTransformComponent->GetTransform())));

		case GameCodeApp::RENDERER_D3D11:
			return shared_ptr<SceneNode>(New D3DLightNode11(m_pOwner->GetActorId(), weakThis, m_Props, &(pTransformComponent->GetTransform())));

		default:
			ASSERT(0 && "Unknown Renderer Implementation in GridRenderComponent");
		}
	}
	return shared_ptr<SceneNode>();
}

void LightRenderComponent::VCreateInheritedXmlElements(TiXmlElement *pBaseElement)
{
	TiXmlElement* pSceneNode = New TiXmlElement("Light");

	// Attenuation
	TiXmlElement* pAttenuation = New TiXmlElement("Attenuation");
	pAttenuation->SetAttribute("const", ToStr(m_Props.m_Attenuation[0]).c_str());
	pAttenuation->SetAttribute("linear", ToStr(m_Props.m_Attenuation[1]).c_str());
	pAttenuation->SetAttribute("exp", ToStr(m_Props.m_Attenuation[2]).c_str());
	pSceneNode->LinkEndChild(pAttenuation);

	// Shape
	TiXmlElement* pShape = New TiXmlElement("Shape");
	pShape->SetAttribute("range", ToStr(m_Props.m_Range).c_str());
	pShape->SetAttribute("falloff", ToStr(m_Props.m_Falloff).c_str());
	pShape->SetAttribute("theta", ToStr(m_Props.m_Theta).c_str());
	pShape->SetAttribute("phi", ToStr(m_Props.m_Phi).c_str());
	pSceneNode->LinkEndChild(pShape);

	pBaseElement->LinkEndChild(pSceneNode);
}




SkyRenderComponent::SkyRenderComponent(void)
{
}

bool SkyRenderComponent::VDelegateInit(TiXmlElement* pData)
{
	TiXmlElement* pTexture = pData->FirstChildElement("Texture");
	if (pTexture)
	{
		m_textureResource = pTexture->FirstChild()->Value();
	}
	return true;
}

shared_ptr<SceneNode> SkyRenderComponent::VCreateSceneNode(void)
{
	shared_ptr<SkyNode> sky;
	if (GameCodeApp::GetRendererImpl() == GameCodeApp::RENDERER_D3D9)
	{
		sky = shared_ptr<SkyNode>(New D3DSkyNode9(m_textureResource.c_str()));
	}
	else if (GameCodeApp::GetRendererImpl() == GameCodeApp::RENDERER_D3D11)
	{
		sky = shared_ptr<SkyNode>(New D3DSkyNode11(m_textureResource.c_str()));
	}
	else
	{
		ERROR("Unknown Renderer Implementation in VLoadGameDelegate");
	}
	return sky;
}

void SkyRenderComponent::VCreateInheritedXmlElements(TiXmlElement *pBaseElement)
{
	TiXmlElement* pTextureNode = New TiXmlElement("Texture");
	TiXmlText* pTextureText = New TiXmlText(m_textureResource.c_str());
	pTextureNode->LinkEndChild(pTextureText);
	pBaseElement->LinkEndChild(pTextureNode);
}