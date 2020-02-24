#include"GameCodeStd.h"
#include"PhysicsComponent.h"
#include"TransformComponent.h"
#include"Physics\Physics.h"
#include"Utilities\String.h"
#include"Utilities\Math.h"
#include"GameCode\GameCode.h"


const float DEFAULT_MAX_VELOCITY = 7.5f;
const float DEFAULT_MAX_ANGULAR_VELOCITY = 1.2f;

const char *PhysicsComponent::s_Name = "PhysicsComponent";

PhysicsComponent::PhysicsComponent(void)
{
	m_RigidBodyLocation = Vec3(0.f, 0.f, 0.f);
	m_RigidBodyOrientation = Vec3(0.f, 0.f, 0.f);
	m_RigidBodyScale = Vec3(1.f, 1.f, 1.f);

	m_acceleration = 0;
	m_angularAcceleration = 0;
	m_maxVelocity = DEFAULT_MAX_VELOCITY;
	m_maxAngularVelocity = DEFAULT_MAX_ANGULAR_VELOCITY;
}

PhysicsComponent::~PhysicsComponent(void)
{
	m_pGamePhysics->VRemoveActor(m_pOwner->GetActorId());
}

bool PhysicsComponent::VInit(TiXmlElement* pData)
{
	m_pGamePhysics = g_pApp->m_pGame->VGetGamePhysics();
	if (!m_pGamePhysics)
		return false;

	// Shape
	TiXmlElement* pShape = pData->FirstChildElement("Shape");
	if (pShape)
	{
		m_shape = pShape->FirstChild()->Value();
	}

	// Density
	TiXmlElement* pDensity = pData->FirstChildElement("Density");
	if (pDensity)
		m_density = pDensity->FirstChild()->Value();

	// Material
	TiXmlElement* pMaterial = pData->FirstChildElement("PhysicsMaterial");
	if (pMaterial)
		m_Material = pMaterial->FirstChild()->Value();

	// Initial transform
	TiXmlElement* pRigidBodyTransform = pData->FirstChildElement("RigidBodyTransform");
	if (pRigidBodyTransform)
		BuildRigidBodyTransform(pRigidBodyTransform);

	return true;
}

TiXmlElement* PhysicsComponent::VGenerateXml(void)
{
	TiXmlElement* pBaseElement = New TiXmlElement(VGetName());

	// Shape
	TiXmlElement* pShape = New TiXmlElement("Shape");
	TiXmlText* pShapeText = New TiXmlText(m_shape.c_str());
	pShape->LinkEndChild(pShapeText);
	pBaseElement->LinkEndChild(pShape);

	// Density
	TiXmlElement* pDensity = New TiXmlElement("Density");
	TiXmlText* pDensityText = New TiXmlText(m_density.c_str());
	pDensity->LinkEndChild(pDensityText);
	pBaseElement->LinkEndChild(pDensity);

	// Material
	TiXmlElement* pMaterial = New TiXmlElement("Material");
	TiXmlText* pMaterialText = New TiXmlText(m_Material.c_str());
	pMaterial->LinkEndChild(pMaterialText);
	pBaseElement->LinkEndChild(pMaterial);

	// Rigid body transform
	TiXmlElement* pInitialTransform = New TiXmlElement("RigidBodyTransform");

	// Initial transform -> position
	TiXmlElement* pPosition = New TiXmlElement("Position");
	pPosition->SetAttribute("x", ToStr(m_RigidBodyLocation.x).c_str());
	pPosition->SetAttribute("y", ToStr(m_RigidBodyLocation.y).c_str());
	pPosition->SetAttribute("z", ToStr(m_RigidBodyLocation.z).c_str());
	pInitialTransform->LinkEndChild(pPosition);

	// Initial transform -> orientation
	TiXmlElement* pOrientation = New TiXmlElement("Orientation");
	pOrientation->SetAttribute("yaw", ToStr(m_RigidBodyOrientation.x).c_str());
	pOrientation->SetAttribute("pitch", ToStr(m_RigidBodyOrientation.y).c_str());
	pOrientation->SetAttribute("roll", ToStr(m_RigidBodyOrientation.z).c_str());
	pInitialTransform->LinkEndChild(pOrientation);

	// Initial transform -> scale 
	TiXmlElement* pScale = New TiXmlElement("Scale");
	pScale->SetAttribute("x", ToStr(m_RigidBodyScale.x).c_str());
	pScale->SetAttribute("y", ToStr(m_RigidBodyScale.y).c_str());
	pScale->SetAttribute("z", ToStr(m_RigidBodyScale.z).c_str());
	pInitialTransform->LinkEndChild(pScale);

	pBaseElement->LinkEndChild(pInitialTransform);

	return pBaseElement;
}

void PhysicsComponent::VPostInit(void)
{
	if (m_pOwner)
	{
		if (m_shape == "Sphere")
		{
			m_pGamePhysics->VAddSphere((float)m_RigidBodyScale.x, m_pOwner, m_density, m_Material);
		}
		else if (m_shape == "Box")
		{
			m_pGamePhysics->VAddBox(m_RigidBodyScale, m_pOwner, m_density, m_Material);
		}
		else if (m_shape == "PointCloud")
		{
			ERROR("Not supported yet!");
		}
	}
}

void PhysicsComponent::VUpdate(int deltaMs)
{
	// Get the transform component
	shared_ptr<TransformComponent> pTransformComponent = MakeStrongPtr(m_pOwner->GetComponent<TransformComponent>(TransformComponent::s_Name));
	if (!pTransformComponent)
	{
		ERROR("No transform component!");
		return;
	}

	// Get the direction the object is facing
	Mat4x4 transform = pTransformComponent->GetTransform();

	if (m_acceleration != 0)
	{
		// calculate the acceleration this frame

		float accelerationToApplyThisFrame = m_acceleration / 1000.f * (float)deltaMs;

		//Calculates the speed
		Vec3 velocity(m_pGamePhysics->VGetVelocity(m_pOwner->GetActorId()));
		float velocityScalar = velocity.Length();

		Vec3 direction(transform.GetDirection());
		m_pGamePhysics->VApplyForce(direction, accelerationToApplyThisFrame, m_pOwner->GetActorId());

	}

	if (m_angularAcceleration != 0)
	{
		// Calculate the acceleration this frame
		float angularAccelerationToApplyThisFrame = m_angularAcceleration / 1000.f * (float)deltaMs;
		m_pGamePhysics->VApplyTorque(transform.GetUp(), angularAccelerationToApplyThisFrame, m_pOwner->GetActorId());

	}
}

void PhysicsComponent::BuildRigidBodyTransform(TiXmlElement* pTransformElement)
{
	
	ASSERT(pTransformElement);

	TiXmlElement* pPositionElement = pTransformElement->FirstChildElement("Position");
	if (pPositionElement)
	{
		double x = 0;
		double y = 0;
		double z = 0;
		pPositionElement->Attribute("x", &x);
		pPositionElement->Attribute("y", &y);
		pPositionElement->Attribute("z", &z);
		m_RigidBodyLocation = Vec3(x, y, z);
	}

	TiXmlElement* pOrientationElement = pTransformElement->FirstChildElement("Orientation");
	if (pOrientationElement)
	{
		double yaw = 0;
		double pitch = 0;
		double roll = 0;
		pPositionElement->Attribute("yaw", &yaw);
		pPositionElement->Attribute("pitch", &pitch);
		pPositionElement->Attribute("roll", &roll);
		m_RigidBodyOrientation = Vec3((float)DEGREES_TO_RADIANS(yaw), (float)DEGREES_TO_RADIANS(pitch), (float)DEGREES_TO_RADIANS(roll));
	}

	TiXmlElement* pScaleElement = pTransformElement->FirstChildElement("Scale");
	if (pScaleElement)
	{
		double x = 0;
		double y = 0;
		double z = 0;
		pScaleElement->Attribute("x", &x);
		pScaleElement->Attribute("y", &y);
		pScaleElement->Attribute("z", &z);
		m_RigidBodyScale = Vec3((float)x, (float)y, (float)z);
	}
}

void PhysicsComponent::ApplyForce(const Vec3& direction, float forceNewtons)
{
	m_pGamePhysics->VApplyForce(direction, forceNewtons, m_pOwner->GetActorId());
}

void PhysicsComponent::ApplyTorque(const Vec3& direction, float forceNewtons)
{
	m_pGamePhysics->VApplyTorque(direction, forceNewtons, m_pOwner->GetActorId());
}

bool PhysicsComponent::KinematicMove(const Mat4x4 &transform)
{
	return m_pGamePhysics->VKinematicMove(transform, m_pOwner->GetActorId());
}

void PhysicsComponent::ApplyAcceleration(float acceleration)
{
	m_acceleration = acceleration;
}

void PhysicsComponent::RemoveAcceleration(void)
{
	m_acceleration = 0;
}

void PhysicsComponent::ApplyAngularAcceleration(float acceleration)
{
	m_angularAcceleration = acceleration;
}

void PhysicsComponent::RemoveAngularAcceleration(void)
{
	m_angularAcceleration = 0;
}

Vec3 PhysicsComponent::GetVelocity(void)
{
	return m_pGamePhysics->VGetVelocity(m_pOwner->GetActorId());
}

void PhysicsComponent::SetVelocity(const Vec3& velocity)
{
	m_pGamePhysics->VSetVelocity(m_pOwner->GetActorId(), velocity);
}

void PhysicsComponent::RotateY(float angleRadians)
{
	shared_ptr<TransformComponent> pTransformComponent = MakeStrongPtr(m_pOwner->GetComponent<TransformComponent>(TransformComponent::s_Name));
	if (pTransformComponent)
	{
		Mat4x4 transform = pTransformComponent->GetTransform();
		Vec3 position = transform.GetPosition();

		Mat4x4 rotateY;
		rotateY.BuildRotationY(angleRadians);
		rotateY.SetPosition(position);

		KinematicMove(rotateY);
	}
	else
		ERROR("Attempting to call RotateY() on actor with no transform component");
}

void PhysicsComponent::SetPosition(float x, float y, float z)
{
	shared_ptr<TransformComponent> pTransformComponent = MakeStrongPtr(m_pOwner->GetComponent<TransformComponent>(TransformComponent::s_Name));
	if (pTransformComponent)
	{
		Mat4x4 transform = pTransformComponent->GetTransform();
		Vec3 position = Vec3(x, y, z);
		transform.SetPosition(position);

		KinematicMove(transform);
	}
	else
		ERROR("Attempting to call RotateY() on actor with no trnasform component");
}

void PhysicsComponent::Stop(void)
{
	return m_pGamePhysics->VStopActor(m_pOwner->GetActorId());
}