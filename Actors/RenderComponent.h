#pragma once
#include "ActorComponent.h"
#include"Graphic3D\SceneNode.h"
#include"Graphic3D\light.h"



class RenderComponentInterface : public ActorComponent
{
public:
	
	virtual shared_ptr<SceneNode> VGetSceneNode(void) = 0;
};


class BaseRenderComponent : public RenderComponentInterface
{
protected:
	Color m_color;
	shared_ptr<SceneNode> m_pSceneNode;

public:
	virtual bool VInit(TiXmlElement* pData) override;
	virtual void VPostInit(void) override;
	virtual void VOnChanged(void) override;
	virtual TiXmlElement* VGenerateXml(void) override;
	const Color GetColor() const { return m_color; }

protected:
	// Loads the SceneNode specific data (represented in the <SceneNode> tag)
	virtual bool VDelegateInit(TiXmlElement* pData) { return true; }
	virtual shared_ptr<SceneNode> VCreateSceneNode(void) = 0;  // factory method to create the appropriate scene node
	Color LoadColor(TiXmlElement* pData);

	// Editor stuff
	virtual TiXmlElement* VCreateBaseElement(void) { return New TiXmlElement(VGetName()); }
	virtual void VCreateInheritedXmlElements(TiXmlElement* pBaseElement) = 0;

private:
	virtual shared_ptr<SceneNode> VGetSceneNode(void) override;
};




class MeshRenderComponent : public BaseRenderComponent
{
public:
	static const char *s_Name;
	virtual const char *VGetName() const { return s_Name; }

protected:
	virtual shared_ptr<SceneNode> VCreateSceneNode(void) override;  // factory method to create the appropriate scene node
	virtual void VCreateInheritedXmlElements(TiXmlElement* pBaseElement);
};


class SphereRenderComponent : public BaseRenderComponent
{
	unsigned int m_segments;
	float m_radius;

public:
	static const char *s_Name;
	virtual const char *VGetName() const { return s_Name; }

	SphereRenderComponent(void);

protected:
	virtual bool VDelegateInit(TiXmlElement* pData) override;
	virtual shared_ptr<SceneNode> VCreateSceneNode(void) override;  // factory method to create the appropriate scene node

	virtual void VCreateInheritedXmlElements(TiXmlElement* pBaseElement);
};


class TeapotRenderComponent : public BaseRenderComponent
{
public:
	static const char *s_Name;
	virtual const char *VGetName() const { return s_Name; }

protected:
	virtual shared_ptr<SceneNode> VCreateSceneNode(void) override;  // factory method to create the appropriate scene node

	// Editor stuff
	virtual void VCreateInheritedXmlElements(TiXmlElement* pBaseElement);
};


class GridRenderComponent : public BaseRenderComponent
{
	std::string m_textureResource;
	int m_squares;

public:
	static const char *s_Name;
	virtual const char *VGetName() const { return s_Name; }

	GridRenderComponent(void);
	const char* GetTextureResource() { return m_textureResource.c_str(); }
	const int GetDivision() { return m_squares; }

protected:
	virtual bool VDelegateInit(TiXmlElement* pData) override;
	virtual shared_ptr<SceneNode> VCreateSceneNode(void) override;  // factory method to create the appropriate scene node

	// Editor stuff
	virtual void VCreateInheritedXmlElements(TiXmlElement* pBaseElement);
};


class LightRenderComponent : public BaseRenderComponent
{
	LightProperties m_Props;

public:
	static const char *s_Name;
	virtual const char *VGetName() const { return s_Name; }

	LightRenderComponent(void);

protected:
	virtual bool VDelegateInit(TiXmlElement* pData) override;
	virtual shared_ptr<SceneNode> VCreateSceneNode(void) override;  // factory method to create the appropriate scene node

	// Editor stuff
	virtual void VCreateInheritedXmlElements(TiXmlElement* pBaseElement);
};


class SkyRenderComponent : public BaseRenderComponent
{
	std::string m_textureResource;

public:
	static const char *s_Name;
	virtual const char *VGetName() const { return s_Name; }

	SkyRenderComponent(void);

protected:
	virtual bool VDelegateInit(TiXmlElement* pData) override;
	virtual shared_ptr<SceneNode> VCreateSceneNode(void) override;  // factory method to create the appropriate scene node

	// Editor stuff
	virtual void VCreateInheritedXmlElements(TiXmlElement* pBaseElement);
};