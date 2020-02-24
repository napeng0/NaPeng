#include"GameCodeStd.h"
#include"TransformComponent.h"
#include"Utilities\Math.h"
#include"Utilities\String.h"

const char* TransformComponent::s_Name = "TransformComponent";

bool TransformComponent::VInit(TiXmlElement* pData)
{
	ASSERT(pData);

	Vec3 yawPitchRoll = m_Transform.GetYawPitchRoll();
	yawPitchRoll.x = RADIANS_TO_DEGREES(yawPitchRoll.x);
	yawPitchRoll.y = RADIANS_TO_DEGREES(yawPitchRoll.y);
	yawPitchRoll.z = RADIANS_TO_DEGREES(yawPitchRoll.z);

	Vec3 pos = m_Transform.GetPosition();

	TiXmlElement* pPos = pData->FirstChildElement("Position");
	if (pPos)
	{
		double x = 0;
		double y = 0;
		double z = 0;
		pPos->Attribute("x", &x);
		pPos->Attribute("y", &y);
		pPos->Attribute("z", &z);
		pos = Vec3(x, y, z);
	}

	TiXmlElement* pDir = pData->FirstChildElement("YawPitchRoll");
	if (pDir)
	{
		double yaw = 0;
		double pitch = 0;
		double roll = 0;
		pDir->Attribute("x", &yaw);
		pDir->Attribute("y", &pitch);
		pDir->Attribute("z", &roll);
	}

	Mat4x4 translation;
	translation.BuildTranslation(pos);

	Mat4x4 rotation;
	rotation.BuildYawPitchRoll((float)DEGREES_TO_RADIANS(yawPitchRoll.x), (float)DEGREES_TO_RADIANS(yawPitchRoll.y), (float)DEGREES_TO_RADIANS(yawPitchRoll.z));

	m_Transform = rotation * translation;
	return true;
}


TiXmlElement* TransformComponent::VGenerateXml()
{
	TiXmlElement* pBase = New TiXmlElement(VGetName());

	TiXmlElement* pPos = New TiXmlElement("Position");
	Vec3 pos(m_Transform.GetPosition());
	pPos->SetAttribute("x", ToStr(pos.x).c_str());
	pPos->SetAttribute("y", ToStr(pos.y).c_str());
	pPos->SetAttribute("z", ToStr(pos.z).c_str());
	pBase->LinkEndChild(pPos);

	TiXmlElement* pDir = New TiXmlElement("YawPitchRoll");
	Vec3 dir(m_Transform.GetYawPitchRoll());
	dir.x = RADIANS_TO_DEGREES(dir.x);
	dir.y = RADIANS_TO_DEGREES(dir.y);
	dir.z = RADIANS_TO_DEGREES(dir.z);

	pDir->SetAttribute("x", ToStr(dir.x).c_str());
	pDir->SetAttribute("y", ToStr(dir.y).c_str());
	pDir->SetAttribute("z", ToStr(dir.z).c_str());
	pBase->LinkEndChild(pDir);

	return pBase;
}


