#pragma once
#include "ActorComponent.h"




class TiXmlElement;

class PhysicsComponent : public ActorComponent
{
public:
	const static char *s_Name;
	virtual const char *VGetName() const override { return PhysicsComponent::s_Name; }

public:
	PhysicsComponent(void);
	virtual ~PhysicsComponent(void);
	virtual TiXmlElement* VGenerateXml(void) override;

	// ActorComponent interface
	virtual bool VInit(TiXmlElement* pData) override;
	virtual void VPostInit(void) override;
	virtual void VUpdate(int deltaMs) override;

	// Physics functions
	void ApplyForce(const Vec3& direction, float forceNewtons);
	void ApplyTorque(const Vec3& direction, float forceNewtons);
	bool KinematicMove(const Mat4x4& transform);

	// Acceleration
	void ApplyAcceleration(float acceleration);
	void RemoveAcceleration(void);
	void ApplyAngularAcceleration(float acceleration);
	void RemoveAngularAcceleration(void);

	//Void RotateY(float angleRadians);
	Vec3 GetVelocity(void);
	void SetVelocity(const Vec3& velocity);
	void RotateY(float angleRadians);
	void SetPosition(float x, float y, float z);
	void Stop(void);


protected:
	void BuildRigidBodyTransform(TiXmlElement* pTransformElement);

	float m_acceleration, m_angularAcceleration;
	float m_maxVelocity, m_maxAngularVelocity;

	std::string m_shape;
	std::string m_density;
	std::string m_Material;

	Vec3 m_RigidBodyLocation;		// This isn't world position! This is how the rigid body is offset from the position of the actor.
	Vec3 m_RigidBodyOrientation;
	Vec3 m_RigidBodyScale;

	shared_ptr<IGamePhysics> m_pGamePhysics;
};