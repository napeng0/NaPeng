#include "GameCodeStd.h"
#include"GameCode\GameCode.h"
#include"GameCode\BaseGameLogic.h"
#include"GameCode\interface.h"
#include"Physics.h"
#include"Actors\Actor.h"
#include"EventManager\EventManager.h"
#include"Actors\TransformComponent.h"
#include"ResourceCache\ResCache.h"
#ifndef DISABLE_PHYSICS
#include "Graphic3D\geometry.h"
#include "EventManager\Events.h"
#include "PhysicsDebugDrawer.h"
#include "PhysicsEventListener.h"
#include "3rdParty\Source\GCC4\3rdParty\bullet-2.79\src\btBulletDynamicsCommon.h"
#include "3rdParty\Source\GCC4\3rdParty\bullet-2.79\src\btBulletCollisionCommon.h"
#include <set>
#include <iterator>
#include <map>
#endif



//These class are based on 3rd party bullet SDK developed by Erwin Coumans
//Copyright(c) 2003 - 2006 Erwin Coumans
struct MaterialData
{
	float m_Restitution;
	float m_Friction;

	MaterialData(float restitution, float friction)
	{
		m_Restitution = restitution;
		m_Friction = friction;
	}

	MaterialData(const MaterialData& other)
	{
		m_Restitution = other.m_Restitution;
		m_Friction = other.m_Friction;
	}
};


class NullPhysics : public IGamePhysics
{
public:
	NullPhysics() { }
	virtual ~NullPhysics() { }

	// Initialization and Maintenance of the Physics World
	virtual bool VInitialize() { return true; }
	virtual void VSyncVisibleScene() { };
	virtual void VOnUpdate(float deltaTime) { }

	// Initialization of Physics Objects
	virtual void VAddSphere(float radius, WeakActorPtr pActor, const std::string& densityStr, const std::string& physicsMaterial) { }
	virtual void VAddBox(const Vec3& dimensions, WeakActorPtr pActor, const std::string& densityStr, const std::string& physicsMaterial) { }
	virtual void VAddPointCloud(Vec3* pVerts, int numPoints, WeakActorPtr pActor, const std::string& densityStr, const std::string& physicsMaterial) { }
	virtual void VRemoveActor(ActorId id) { }

	// Debugging
	virtual void VRenderDiagnostics() { }

	// Physics world modifiers
	virtual void VCreateTrigger(WeakActorPtr pActor, const Vec3 &pos, const float dim) { }
	virtual void VApplyForce(const Vec3 &dir, float newtons, ActorId aid) { }
	virtual void VApplyTorque(const Vec3 &dir, float newtons, ActorId aid) { }
	virtual bool VKinematicMove(const Mat4x4 &mat, ActorId aid) { return true; }

	// Physics actor states
	virtual void VRotateY(ActorId actorId, float angleRadians, float time) { }
	virtual float VGetOrientationY(ActorId actorId) { return 0.0f; }
	virtual void VStopActor(ActorId actorId) { }
	virtual Vec3 VGetVelocity(ActorId actorId) { return Vec3(); }
	virtual void VSetVelocity(ActorId actorId, const Vec3& vel) { }
	virtual Vec3 VGetAngularVelocity(ActorId actorId) { return Vec3(); }
	virtual void VSetAngularVelocity(ActorId actorId, const Vec3& vel) { }
	virtual void VTranslate(ActorId actorId, const Vec3& vec) { }
	virtual void VSetTransform(const ActorId id, const Mat4x4& mat) { }
	virtual Mat4x4 VGetTransform(const ActorId id) { return Mat4x4::s_Identity; }
};


static btVector3 Vec3TobtVector3(Vec3 const & vec3)
{
	return btVector3(vec3.x, vec3.y, vec3.z);
}

static Vec3 btVector3ToVec3(btVector3 const & btvec)
{
	return Vec3(btvec.x(), btvec.y(), btvec.z());
}


static btTransform Mat4x4TobtTransform(Mat4x4 const & mat)
{
	// Convert from Mat4x4 (GameCode) to btTransform (Bullet)
	btMatrix3x3 bulletRotation;
	btVector3 bulletPosition;

	// Copy position, Mat4x4 is row-major while btMatrix3x3 is column-major
	//Transpose when it is copied
	for (int row = 0; row < 3; ++row)
		for (int column = 0; column < 3; ++column)
			bulletRotation[row][column] = mat.m[column][row]; 

	
	for (int column = 0; column < 3; ++column)
		bulletPosition[column] = mat.m[3][column];

	return btTransform(bulletRotation, bulletPosition);
}


static Mat4x4 btTransformToMat4x4(btTransform const & trans)
{
	Mat4x4 returnValue = Mat4x4::s_Identity;

	// Convert from btTransform (Bullet) to Mat4x4 (GameCode)
	btMatrix3x3 const & bulletRotation = trans.getBasis();
	btVector3 const & bulletPosition = trans.getOrigin();

	// Copy position, Mat4x4 is row-major while btMatrix3x3 is column-major
	//Transpose when it is copied
	for (int row = 0; row < 3; ++row)
		for (int column = 0; column < 3; ++column)
			returnValue.m[row][column] = bulletRotation[column][row];

// Copy position
	for (int column = 0; column < 3; ++column)
		returnValue.m[3][column] = bulletPosition[column];

	return returnValue;
}


//This struct anc be used to represent the center of actor mass
struct ActorMotionState : public btMotionState
{
	Mat4x4 m_Transform;

	ActorMotionState(Mat4x4 const & startingTransform)
		: m_Transform(startingTransform) { }

	// btMotionState interface:  Bullet calls these
	virtual void getWorldTransform(btTransform& worldTrans) const
	{
		worldTrans = Mat4x4TobtTransform(m_Transform);
	}

	virtual void setWorldTransform(const btTransform& worldTrans)
	{
		m_Transform = btTransformToMat4x4(worldTrans);
	}
};





class BulletPhysics : public IGamePhysics, Noncopyable
{
	
	btDynamicsWorld*                 m_DynamicsWorld;//Used to Decide bullet behavior
	btBroadphaseInterface*           m_Broadphase;//Used toRoughly detect possible collision
	btCollisionDispatcher*           m_Dispatcher;//Used to fire collsion events
	btConstraintSolver*              m_Constraint;//Used to apply constriant forces
	btDefaultCollisionConfiguration* m_CollisionConfiguration;
	BulletDebugDrawer*               m_DebugDrawer;

	typedef std::map<std::string, float> DensityTable;
	typedef std::map<std::string, MaterialData> MaterialTable;
	DensityTable m_DensityTable;
	MaterialTable m_MaterialTable;

	typedef std::map<ActorId, btRigidBody*> ActorIDToBulletRigidBodyMap;
	ActorIDToBulletRigidBodyMap m_ActorIdToRigidBody;

	typedef std::map<const btRigidBody*, ActorId> BulletRigidBodyToActorIDMap;
	BulletRigidBodyToActorIDMap m_RigidBodyToActorId;

	typedef std::pair< const btRigidBody*, const btRigidBody* > CollisionPair;
	typedef std::set< CollisionPair > CollisionPairs;
	CollisionPairs m_LastTickCollisionPairs;//A pair of actors colliding each other

	void LoadXml();
	float LookupSpecificGravity(const std::string& densityStr);
	MaterialData LookupMaterialData(const std::string& materialStr);

	
	
	// Collision pair events
	void SendCollisionPairAddEvent(const btPersistentManifold* manifold, const btRigidBody* bodyA, const btRigidBody* bodyB);
	void SendCollisionPairRemoveEvent(const btRigidBody* bodyA, const btRigidBody* bodyB);

	btRigidBody* FindBulletRigidBody(ActorId id) const;
	ActorId FindActorId(const btRigidBody*) const;

	void AddShape(StrongActorPtr pActor, btCollisionShape* pShape, float mass, const std::string& physicsMaterial);
	void RemoveCollisionObject(btCollisionObject* object);

	static void BulletInternalTickCallback( btDynamicsWorld* world, const btScalar timeStep);

public:
	BulletPhysics();				
	virtual ~BulletPhysics();

	// Initialiazation and Maintenance of the Physics World
	virtual bool VInitialize() override;
	virtual void VSyncVisibleScene() override;
	virtual void VOnUpdate(float deltaSeconds) override;

	// Initialization of Physics Objects
	virtual void VAddSphere(float radius, WeakActorPtr pActor, const std::string& densityStr, const std::string& physicsMaterial) override;
	virtual void VAddBox(const Vec3& dimensions, WeakActorPtr pActor, const std::string& densityStr, const std::string& physicsMaterial) override;
	virtual void VAddPointCloud(Vec3* pVerts, int numPoints, WeakActorPtr pActor, const std::string& densityStr, const std::string& physicsMaterial) override;
	virtual void VRemoveActor(ActorId id) override;

	// Debugging
	virtual void VRenderDiagnostics() override;

	// Physics world modifiers
	virtual void VCreateTrigger(WeakActorPtr pActor, const Vec3 &pos, const float dim) override;
	virtual void VApplyForce(const Vec3&dir, float newtons, ActorId aid) override;
	virtual void VApplyTorque(const Vec3&dir, float newtons, ActorId aid) override;
	virtual bool VKinematicMove(const Mat4x4 &mat, ActorId aid) override;

	virtual void VRotateY(ActorId actorId, float angleRadians, float time);
	virtual float VGetOrientationY(ActorId actorId);
	virtual void VStopActor(ActorId actorId);
	virtual Vec3 VGetVelocity(ActorId actorId);
	virtual void VSetVelocity(ActorId actorId, const Vec3& vel);
	virtual Vec3 VGetAngularVelocity(ActorId actorId);
	virtual void VSetAngularVelocity(ActorId actorId, const Vec3& vel);
	virtual void VTranslate(ActorId actorId, const Vec3& vec);

	virtual void VSetTransform(const ActorId id, const Mat4x4& mat);

	virtual Mat4x4 VGetTransform(const ActorId id);
};



BulletPhysics::BulletPhysics()
{
	//Register event type
	REGISTER_EVENT(EvtData_PhysTrigger_Enter);
	REGISTER_EVENT(EvtData_PhysTrigger_Leave);
	REGISTER_EVENT(EvtData_PhysCollision);
	REGISTER_EVENT(EvtData_PhysSeparation);
}


BulletPhysics::~BulletPhysics()
{
	for (int i = ((btCollisionWorld*)m_DynamicsWorld)->getNumCollisionObjects() - 1; i >= 0; --i)
	{
		btCollisionObject* obj = ((btCollisionWorld*)m_DynamicsWorld)->getCollisionObjectArray()[i];

		RemoveCollisionObject(obj);
	}

	m_RigidBodyToActorId.clear();

	SAFE_DELETE(m_DebugDrawer);
	SAFE_DELETE(m_DynamicsWorld);
	SAFE_DELETE(m_Constraint);
	SAFE_DELETE(m_Broadphase);
	SAFE_DELETE(m_Dispatcher);
	SAFE_DELETE(m_CollisionConfiguration);
}


void BulletPhysics::LoadXml()
{
	//Load the physics config file
	TiXmlElement* pRoot = XmlResourceLoader::LoadAndReturnRootXmlElement("config\\Physics.xml");
	ASSERT(pRoot);

	//Load all materials
	TiXmlElement* pParentNode = pRoot->FirstChildElement("PhysicsMaterials");
	ASSERT(pParentNode);
	for (TiXmlElement* pNode = pParentNode->FirstChildElement(); pNode; pNode = pNode->NextSiblingElement())
	{
		double restitution = 0;
		double friction = 0;
		pNode->Attribute("restitution", &restitution);
		pNode->Attribute("friction", &friction);
		m_MaterialTable.insert(std::make_pair(pNode->Value(), MaterialData((float)restitution, (float)friction)));
	}

	// Load all densities
	pParentNode = pRoot->FirstChildElement("DensityTable");
	ASSERT(pParentNode);
	for (TiXmlElement* pNode = pParentNode->FirstChildElement(); pNode; pNode = pNode->NextSiblingElement())
	{
		m_DensityTable.insert(std::make_pair(pNode->Value(), (float)atof(pNode->FirstChild()->Value())));
	}
}


float BulletPhysics::LookupSpecificGravity(const std::string& densityStr)
{
	float density = 0;
	auto densityIt = m_DensityTable.find(densityStr);
	if (densityIt != m_DensityTable.end())
		density = densityIt->second;
	else
	{
		ERROR("Density not found!");
		return 0.0f;
	}
	return density;
}


MaterialData BulletPhysics::LookupMaterialData(const std::string& materialStr)
{
	auto materialIt = m_MaterialTable.find(materialStr);
	if (materialIt != m_MaterialTable.end())
		return materialIt->second;
	else
		return MaterialData(0, 0);
}


void BulletPhysics::SendCollisionPairAddEvent(const btPersistentManifold* manifold, const btRigidBody* bodyA, const btRigidBody* bodyB)
{
	if (bodyA->getUserPointer() || bodyB->getUserPointer())
	{

		// Figure out which actor is the trigger
		const btRigidBody* pTriggerBody,* pOtherBody;

		if (bodyA->getUserPointer())
		{
			pTriggerBody = bodyA;
			pOtherBody = bodyB;
		}
		else
		{
			pOtherBody = bodyA;
			pTriggerBody = bodyB;
		}

		// Send the trigger event.
		const int triggerId = *(int*)(pTriggerBody->getUserPointer());
		shared_ptr<EvtData_PhysTrigger_Enter> pEvent(New EvtData_PhysTrigger_Enter(triggerId, FindActorId(pOtherBody)));
		IEventManager::Get()->VQueueEvent(pEvent);
	}
	else
	{
		ActorId idA = FindActorId(bodyA);
		ActorId idB = FindActorId(bodyB);

		if (idA == INVALID_ACTOR_ID || idB == INVALID_ACTOR_ID)
		{
			// Something is colliding with a non-actor.
			return;
		}

		// This pair of colliding objects is new.  send a collision-begun event
		Vec3List collisionPoints;
		Vec3 sumNormalForce;
		Vec3 sumFrictionForce;

		for (int i = 0; i < manifold->getNumContacts(); ++i)
		{
			const btManifoldPoint& point = manifold->getContactPoint(i);

			collisionPoints.push_back(btVector3ToVec3(point.getPositionWorldOnB()));

			sumNormalForce += btVector3ToVec3(point.m_combinedRestitution * point.m_normalWorldOnB);
			sumFrictionForce += btVector3ToVec3(point.m_combinedFriction * point.m_lateralFrictionDir1);
		}

		// send the event for the game
		shared_ptr<EvtData_PhysCollision> pEvent(New EvtData_PhysCollision(idA, idB, sumNormalForce, sumFrictionForce, collisionPoints));
		IEventManager::Get()->VQueueEvent(pEvent);
	}
}


void BulletPhysics::SendCollisionPairRemoveEvent(const btRigidBody* bodyA, const btRigidBody* bodyB)
{
	if (bodyA->getUserPointer() || bodyB->getUserPointer())
	{
		// Figure out which actor is the trigger
		const btRigidBody* triggerBody,* otherBody;

		if (bodyA->getUserPointer())
		{
			triggerBody = bodyA;
			otherBody = bodyB;
		}
		else
		{
			otherBody = bodyA;
			triggerBody = bodyB;
		}

		// Send the trigger event.
		const int triggerId = *(int*)(triggerBody->getUserPointer());
		shared_ptr<EvtData_PhysTrigger_Leave> pEvent(New EvtData_PhysTrigger_Leave(triggerId, FindActorId(otherBody)));
		IEventManager::Get()->VQueueEvent(pEvent);
	}
	else
	{
		ActorId idA = FindActorId(bodyA);
		ActorId idB = FindActorId(bodyB);

		if (idA == INVALID_ACTOR_ID || idB == INVALID_ACTOR_ID)
			return;

		shared_ptr<EvtData_PhysSeparation> pEvent(New EvtData_PhysSeparation(idA, idB));
		IEventManager::Get()->VQueueEvent(pEvent);
	}
}



btRigidBody* BulletPhysics::FindBulletRigidBody(const ActorId id) const
{
	ActorIDToBulletRigidBodyMap::const_iterator it = m_ActorIdToRigidBody.find(id);
	if (it != m_ActorIdToRigidBody.end())
		return it->second;

	return NULL;
}

ActorId BulletPhysics::FindActorId(const btRigidBody * body) const
{
	BulletRigidBodyToActorIDMap::const_iterator it = m_RigidBodyToActorId.find(body);
	if (it != m_RigidBodyToActorId.end())
		return it->second;

	return ActorId();
}





void BulletPhysics::AddShape(StrongActorPtr pActor, btCollisionShape* shape, float mass, const std::string& physicsMaterial)
{
	ASSERT(pActor);

	ActorId actorID = pActor->GetActorId();
	ASSERT(m_ActorIdToRigidBody.find(actorID) == m_ActorIdToRigidBody.end() && "Actor with more than one physics body?");

	// Lookup the material
	MaterialData material(LookupMaterialData(physicsMaterial));

	// LocalInertia defines how the object's mass is distributed
	btVector3 localInertia(0.f, 0.f, 0.f);
	if (mass > 0.f)
		shape->calculateLocalInertia(mass, localInertia);


	Mat4x4 transform = Mat4x4::s_Identity;
	shared_ptr<TransformComponent> pTransformComponent = MakeStrongPtr(pActor->GetComponent<TransformComponent>(TransformComponent::s_Name));
	ASSERT(pTransformComponent);
	if (pTransformComponent)
	{
		transform = pTransformComponent->GetTransform();
	}
	else
	{
		ERROR("Physics doesn't work without transform!");
		return;
	}

	// Set the initial transform of the body from the actor
	ActorMotionState* pMotionState = New ActorMotionState(transform);

	btRigidBody::btRigidBodyConstructionInfo info(mass, pMotionState, shape, localInertia);

	// Set up the materal properties
	info.m_restitution = material.m_Restitution;
	info.m_friction = material.m_Friction;

	btRigidBody* body = new btRigidBody(info);

	m_DynamicsWorld->addRigidBody(body);

	// add it to the collection to be checked for changes in VSyncVisibleScene
	m_ActorIdToRigidBody[actorID] = body;
	m_RigidBodyToActorId[body] = actorID;
}


void BulletPhysics::RemoveCollisionObject(btCollisionObject* object)
{
	m_DynamicsWorld->removeCollisionObject(object);

	for (CollisionPairs::iterator it = m_LastTickCollisionPairs.begin();
		it != m_LastTickCollisionPairs.end(); )
	{
		CollisionPairs::iterator next = it;
		++next;

		if (it->first == object || it->second == object)
		{
			SendCollisionPairRemoveEvent(it->first, it->second);
			m_LastTickCollisionPairs.erase(it);
		}

		it = next;
	}

	if (btRigidBody* body = btRigidBody::upcast(object))
	{
		delete body->getMotionState();
		delete body->getCollisionShape();
		delete body->getUserPointer();

		for (int i = body->getNumConstraintRefs() - 1; i >= 0; --i)
		{
			btTypedConstraint* constraint = body->getConstraintRef(i);
			m_DynamicsWorld->removeConstraint(constraint);
			delete constraint;
		}
	}

	delete object;
}


void BulletPhysics::BulletInternalTickCallback(btDynamicsWorld* world, const btScalar timeStep)
{
	ASSERT(world);

	ASSERT(world->getWorldUserInfo());
	BulletPhysics* bulletPhysics = (BulletPhysics*)(world->getWorldUserInfo());

	CollisionPairs collisionPairs;

	// Look at all existing contacts
	btDispatcher* dispatcher = (btDispatcher* )world->getDispatcher();
	for (int manifoldId = 0; manifoldId < dispatcher->getNumManifolds(); ++manifoldId)
	{
		// Get the "manifold", which is the set of data corresponding to a contact point
		//   between two physics objects
		btPersistentManifold* manifold = dispatcher->getManifoldByIndexInternal(manifoldId);
		ASSERT(manifold);

		btRigidBody* bodyA = (btRigidBody*)(manifold->getBody0());
		btRigidBody* bodyB = (btRigidBody*)(manifold->getBody1());

		// Always create the pair in a predictable order
		bool swapped = bodyA > bodyB;

		btRigidBody* sortedBodyA = swapped ? bodyB : bodyA;
		btRigidBody* sortedBodyB = swapped ? bodyA : bodyB;

		CollisionPair thisPair = std::make_pair(sortedBodyA, sortedBodyB);
		collisionPairs.insert(thisPair);

		if (bulletPhysics->m_LastTickCollisionPairs.find(thisPair) == bulletPhysics->m_LastTickCollisionPairs.end())
		{
			// this is a new contact, which wasn't in our list before.  send an event to the game.
			bulletPhysics->SendCollisionPairAddEvent(manifold, bodyA, bodyB);
		}
	}

	CollisionPairs removedCollisionPairs;

	std::set_difference(bulletPhysics->m_LastTickCollisionPairs.begin(), bulletPhysics->m_LastTickCollisionPairs.end(),
		collisionPairs.begin(), collisionPairs.end(),
		std::inserter(removedCollisionPairs, removedCollisionPairs.begin()));

	for (CollisionPairs::const_iterator it = removedCollisionPairs.begin(),
		end = removedCollisionPairs.end(); it != end; ++it)
	{
		const btRigidBody* bodyA = it->first;
		const btRigidBody* bodyB = it->second;

		bulletPhysics->SendCollisionPairRemoveEvent(bodyA, bodyB);
	}

	bulletPhysics->m_LastTickCollisionPairs = collisionPairs;
}




bool BulletPhysics::VInitialize()
{
	LoadXml();

	m_CollisionConfiguration = New btDefaultCollisionConfiguration();

	m_Dispatcher = New btCollisionDispatcher(m_CollisionConfiguration);

	//Roughly detecte possible collision
	m_Broadphase = New btDbvtBroadphase();

	// Manages constraints which apply forces to the physics simulation.
	m_Constraint = New btSequentialImpulseConstraintSolver;

	// This is the main Bullet interface .
	m_DynamicsWorld = New btDiscreteDynamicsWorld(m_Dispatcher,
		m_Broadphase,
		m_Constraint,
		m_CollisionConfiguration);

	m_DebugDrawer = New BulletDebugDrawer;
	m_DebugDrawer->ReadOptions();

	if (!m_CollisionConfiguration || !m_Dispatcher || !m_Broadphase ||
		!m_Constraint || !m_DynamicsWorld || !m_DebugDrawer)
	{
		ERROR("BulletPhysics::VInitialize failed!");
		return false;
	}

	m_DynamicsWorld->setDebugDrawer(m_DebugDrawer);


	
	m_DynamicsWorld->setInternalTickCallback(BulletInternalTickCallback);
	m_DynamicsWorld->setWorldUserInfo(this);

	return true;
}



void BulletPhysics::VSyncVisibleScene()
{
	// Keep physics & graphics in sync
	//  If there is a change, send the appropriate event for the game system.
	for (ActorIDToBulletRigidBodyMap::const_iterator it = m_ActorIdToRigidBody.begin();
		it != m_ActorIdToRigidBody.end();
		++it)
	{
		const ActorId id = it->first;

		// get the MotionState.  this object is updated by Bullet.
		// it's safe to cast the btMotionState to ActorMotionState, because all the bodies in m_ActorIdToRigidBody
		//   were created through AddShape()
		const ActorMotionState* motionState = (ActorMotionState*)(it->second->getMotionState());
		ASSERT(motionState);

		StrongActorPtr pActor = MakeStrongPtr(g_pApp->m_pGame->VGetActor(id));
		if (pActor && motionState)
		{
			shared_ptr<TransformComponent> pTransformComponent = MakeStrongPtr(pActor->GetComponent<TransformComponent>(TransformComponent::s_Name));
			if (pTransformComponent)
			{
				if (pTransformComponent->GetTransform() != motionState->m_Transform)
				{
					// Bullet has moved the actor's physics object.  Sync the transform and inform the game an actor has moved
					pTransformComponent->SetTransform(motionState->m_Transform);
					shared_ptr<EvtData_Move_Actor> pEvent(New EvtData_Move_Actor(id, motionState->m_Transform));
					IEventManager::Get()->VQueueEvent(pEvent);
				}
			}
		}
	}
}




void BulletPhysics::VOnUpdate(const float deltaSeconds)
{
	// Update physics 4 time a tick
	m_DynamicsWorld->stepSimulation(deltaSeconds, 4);
}


void BulletPhysics::VAddSphere(const float radius, WeakActorPtr pActor, const std::string& densityStr, const std::string& physicsMaterial)
{
	StrongActorPtr pStrongActor = MakeStrongPtr(pActor);
	if (!pStrongActor)
	{
		ERROR("Can't add physics component, invalid actor!");
		return;
	}

	// Create the collision body
	btSphereShape* collisionShape = new btSphereShape(radius);

	// Calculate absolute mass from specificGravity
	float specificGravity = LookupSpecificGravity(densityStr);
	const float volume = (4.f / 3.f) * PI * radius * radius * radius;
	const btScalar mass = volume * specificGravity;

	AddShape(pStrongActor, collisionShape, mass, physicsMaterial);
}


void BulletPhysics::VAddBox(const Vec3& dimensions, WeakActorPtr pActor, const std::string& densityStr, const std::string& physicsMaterial)
{
	StrongActorPtr pStrongActor = MakeStrongPtr(pActor);
	if (!pStrongActor)
	{
		ERROR("Can't add physics component, invalid actor!");
		return;
	}

	// Create the collision body
	btBoxShape* boxShape = new btBoxShape(Vec3TobtVector3(dimensions));

	// Calculate absolute mass from specificGravity
	float specificGravity = LookupSpecificGravity(densityStr);
	const float volume = dimensions.x * dimensions.y * dimensions.z;
	btScalar const mass = volume * specificGravity;

	AddShape(pStrongActor, boxShape, mass, physicsMaterial);
}


void BulletPhysics::VAddPointCloud(Vec3 *pVerts, int numPoints, WeakActorPtr pActor, const std::string& densityStr, const std::string& physicsMaterial)
{
	StrongActorPtr pStrongActor = MakeStrongPtr(pActor);
	if (!pStrongActor)
	{
		ERROR("Can't add physics component, invalid actor!");
		return;
	}

	btConvexHullShape* shape = new btConvexHullShape();

	// Add the points to the shape
	for (int i = 0; i < numPoints; ++i)
		shape->addPoint(Vec3TobtVector3(pVerts[i]));

	// Approximate absolute mass using bounding box
	btVector3 aabbMin(0, 0, 0), aabbMax(0, 0, 0);
	shape->getAabb(btTransform::getIdentity(), aabbMin, aabbMax);

	const btVector3 aabbExtents = aabbMax - aabbMin;

	float specificGravity = LookupSpecificGravity(densityStr);
	const float volume = aabbExtents.x() * aabbExtents.y() * aabbExtents.z();
	const btScalar mass = volume * specificGravity;

	AddShape(pStrongActor, shape, mass, physicsMaterial);
}


void BulletPhysics::VRemoveActor(ActorId id)
{
	if (btRigidBody* body = FindBulletRigidBody(id))
	{
		// Destroy the body and all its components
		RemoveCollisionObject(body);
		m_ActorIdToRigidBody.erase(id);
		m_RigidBodyToActorId.erase(body);
	}
}


void BulletPhysics::VRenderDiagnostics()
{
	m_DynamicsWorld->debugDrawWorld();
}


void BulletPhysics::VCreateTrigger(WeakActorPtr pActor, const Vec3 &pos, const float dim)
{
	StrongActorPtr pStrongActor = MakeStrongPtr(pActor);
	if (!pStrongActor)
	{
		ERROR("Can't add physics component, invalid actor!");
		return;
	}

	btBoxShape* boxShape = new btBoxShape(Vec3TobtVector3(Vec3(dim, dim, dim)));

	// Triggers are immoveable.  
	const btScalar mass = 0;

	// Set the initial position of the body from the actor
	Mat4x4 triggerTrans = Mat4x4::s_Identity;
	triggerTrans.SetPosition(pos);
	ActorMotionState* motionState = New ActorMotionState(triggerTrans);

	btRigidBody::btRigidBodyConstructionInfo info(mass, motionState, boxShape, btVector3(0, 0, 0));
	btRigidBody* body = new btRigidBody(info);

	m_DynamicsWorld->addRigidBody(body);

	// A Trigger is just a box that doesn't collide with anything.  That's what "CF_NO_CONTACT_RESPONSE" indicates.
	body->setCollisionFlags(body->getCollisionFlags() | btRigidBody::CF_NO_CONTACT_RESPONSE);

	m_ActorIdToRigidBody[pStrongActor->GetActorId()] = body;
	m_RigidBodyToActorId[body] = pStrongActor->GetActorId();
}


void BulletPhysics::VApplyForce(const Vec3& dir, float newtons, ActorId aid)
{
	if (btRigidBody* body = FindBulletRigidBody(aid))
	{
		body->setActivationState(DISABLE_DEACTIVATION);

		const btVector3 force(dir.x * newtons,
			dir.y * newtons,
			dir.z * newtons);

		body->applyCentralImpulse(force);
	}
}


void BulletPhysics::VApplyTorque(const Vec3& dir, float force, ActorId aid)
{
	if (btRigidBody* body = FindBulletRigidBody(aid))
	{
		body->setActivationState(DISABLE_DEACTIVATION);

		const btVector3 torque(dir.x * force,
			dir.y * force,
			dir.z * force);

		body->applyTorqueImpulse(torque);
	}
}


bool BulletPhysics::VKinematicMove(const Mat4x4& mat, ActorId aid)
{
	if (btRigidBody* body = FindBulletRigidBody(aid))
	{
		body->setActivationState(DISABLE_DEACTIVATION);

		// Warp the body to the new position
		body->setWorldTransform(Mat4x4TobtTransform(mat));
		return true;
	}

	return false;
}



void BulletPhysics::VRotateY(const ActorId actorId, const float angleRadians, const float time)
{
	btRigidBody* pRigidBody = FindBulletRigidBody(actorId);
	ASSERT(pRigidBody);

	// Create a transform to represent the additional turning this frame
	btTransform angleTransform;
	angleTransform.setIdentity();
	angleTransform.getBasis().setEulerYPR(0, angleRadians, 0); // Rotation around body Y-axis

	// Concatenate the transform onto the body's transform
	pRigidBody->setCenterOfMassTransform(pRigidBody->getCenterOfMassTransform() * angleTransform);
}



float BulletPhysics::VGetOrientationY(ActorId actorId)
{
	btRigidBody* pRigidBody = FindBulletRigidBody(actorId);
	ASSERT(pRigidBody);

	const btTransform& actorTransform = pRigidBody->getCenterOfMassTransform();
	btMatrix3x3 actorRotationMat(actorTransform.getBasis());  

	btVector3 startingVec(0, 0, 1);
	btVector3 endingVec = actorRotationMat * startingVec; 

	endingVec.setY(0);  // we only care about rotation on the XZ plane

	const float endingVecLength = endingVec.length();
	if (endingVecLength < 0.001)
	{
		return 0;
	}
	else
	{
		btVector3 crossVec = startingVec.cross(endingVec);
		float sign = crossVec.getY() > 0 ? 1.0f : -1.0f;
		return (acosf(startingVec.dot(endingVec) / endingVecLength) * sign);
	}

	return FLT_MAX;  // Failed
}


void BulletPhysics::VStopActor(ActorId actorId)
{
	VSetVelocity(actorId, Vec3(0.f, 0.f, 0.f));
}


Vec3 BulletPhysics::VGetVelocity(ActorId actorId)
{
	btRigidBody* pRigidBody = FindBulletRigidBody(actorId);
	ASSERT(pRigidBody);
	if (!pRigidBody)
		return Vec3();
	btVector3 btVel = pRigidBody->getLinearVelocity();
	return btVector3ToVec3(btVel);
}


void BulletPhysics::VSetVelocity(ActorId actorId, const Vec3& vel)
{
	btRigidBody* pRigidBody = FindBulletRigidBody(actorId);
	ASSERT(pRigidBody);
	if (!pRigidBody)
		return;
	btVector3 btVel = Vec3TobtVector3(vel);
	pRigidBody->setLinearVelocity(btVel);
}


void BulletPhysics::VSetAngularVelocity(ActorId actorId, const Vec3& vel)
{
	btRigidBody* pRigidBody = FindBulletRigidBody(actorId);
	ASSERT(pRigidBody);
	if (!pRigidBody)
		return;
	btVector3 btVel = Vec3TobtVector3(vel);
	pRigidBody->setAngularVelocity(btVel);
}

Vec3 BulletPhysics::VGetAngularVelocity(ActorId actorId)
{
	btRigidBody* pRigidBody = FindBulletRigidBody(actorId);
	ASSERT(pRigidBody);
	if (!pRigidBody)
		return Vec3();
	btVector3 btVel = pRigidBody->getAngularVelocity();
	return btVector3ToVec3(btVel);
}



void BulletPhysics::VTranslate(ActorId actorId, const Vec3& vec)
{
	btRigidBody* pRigidBody = FindBulletRigidBody(actorId);
	ASSERT(pRigidBody);
	btVector3 btVec = Vec3TobtVector3(vec);
	pRigidBody->translate(btVec);
}


void BulletPhysics::VSetTransform(ActorId actorId, const Mat4x4& mat)
{
	VKinematicMove(mat, actorId);
}



Mat4x4 BulletPhysics::VGetTransform(const ActorId id)
{
	btRigidBody* pRigidBody = FindBulletRigidBody(id);
	ASSERT(pRigidBody);

	const btTransform& actorTransform = pRigidBody->getCenterOfMassTransform();
	return btTransformToMat4x4(actorTransform);
}