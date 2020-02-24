#pragma once
#include"ActorComponent.h"

class TransformComponent : public ActorComponent
{
	Mat4x4 m_Transform;

public:
	static const char* s_Name;

public:
	TransformComponent() { m_Transform = Mat4x4::s_Identity; }

	virtual const char* VGetName() const { return s_Name; }
	virtual bool VInit(TiXmlElement* pData) override;
	virtual TiXmlElement* VGenerateXml() override;

	Mat4x4 GetTransform() const { return m_Transform; }
	void SetTransform(const Mat4x4& newtrans) { m_Transform = newtrans; }
	Vec3 GetPosition() const { return m_Transform.GetPosition(); }
	void SetPosition(const Vec3& pos) { m_Transform.SetPosition(pos); }
	Vec3 GetLookAt() const { return m_Transform.GetDirection(); }

};