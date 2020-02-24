#pragma once
#include"EventManager\EventManager.h"
#include"GameCode\GameCode.h"
#include"LUA\ScriptEvent.h"


class EvtData_PhysTrigger_Enter : public BaseEventData
{
	int m_TriggerId;
	ActorId m_Other;

public:
	static const EventType s_EventType;

public:
	
	EvtData_PhysTrigger_Enter()
	{
		m_TriggerId = -1;
		m_Other = INVALID_ACTOR_ID;
	}

	explicit EvtData_PhysTrigger_Enter(int triggerId, ActorId other)
	{
		m_TriggerId = triggerId;
		m_Other = other;
	}

	virtual const EventType& VGetEventType() const
	{
		return s_EventType;
	}

	virtual const char* VGetName() const
	{
		return "EvtData_PhysTrigger_Enter";
	}

	IEventDataPtr VCopy() const
	{
		return IEventDataPtr(New EvtData_PhysTrigger_Enter(m_TriggerId, m_Other));

	}

	int GetTriggerId() const
	{
		return m_TriggerId;
	}

	ActorId GetOtherActor() const
	{
		return m_Other;
	}
};


class EvtData_PhysTrigger_Leave : public BaseEventData
{
	int m_TriggerId;
	ActorId m_Other;

public:
	static const EventType s_EventType;

public:

	EvtData_PhysTrigger_Leave()
	{
		m_TriggerId = -1;
		m_Other = INVALID_ACTOR_ID;
	}

	explicit EvtData_PhysTrigger_Leave(int triggerId, ActorId other)
	{
		m_TriggerId = triggerId;
		m_Other = other;
	}

	virtual const EventType& VGetEventType() const
	{
		return s_EventType;
	}

	virtual const char* VGetName() const
	{
		return "EvtData_PhysTrigger_Leave";
	}

	IEventDataPtr VCopy() const
	{
		return IEventDataPtr(New EvtData_PhysTrigger_Leave(m_TriggerId, m_Other));

	}

	int GetTriggerId() const
	{
		return m_TriggerId;
	}

	ActorId GetOtherActor() const
	{
		return m_Other;
	}
};


class EvtData_PhysCollision : public ScriptEvent
{
	ActorId m_ActorA;
	ActorId m_ActorB;
	Vec3 m_SumNormalForce;
	Vec3 m_SumFrictionForce;
	Vec3List m_CollisionPoints;

public:
	static const EventType s_EventType;

public:

	EvtData_PhysCollision()
	{
		m_ActorA = INVALID_ACTOR_ID;
		m_ActorB = INVALID_ACTOR_ID;
		m_SumNormalForce = Vec3(0.0f, 0.0f, 0.0f);
		m_SumFrictionForce= Vec3(0.0f, 0.0f, 0.0f);
	}

	explicit EvtData_PhysCollision(ActorId actorA, ActorId actorB, Vec3 sumNormalForce, Vec3 sumFrictionForce, Vec3List collisionPoints)
	{
		m_ActorA = actorA;
		m_ActorB = actorB;
		m_SumNormalForce = sumNormalForce;
		m_SumFrictionForce = sumFrictionForce;
		m_CollisionPoints = collisionPoints;
	}

	virtual const EventType& VGetEventType() const
	{
		return s_EventType;
	}

	virtual const char* VGetName() const
	{
		return "EvtData_PhysCollision";
	}

	IEventDataPtr VCopy() const
	{
		return IEventDataPtr(New EvtData_PhysCollision(m_ActorA, m_ActorB, m_SumNormalForce, m_SumFrictionForce, m_CollisionPoints));
	}

	ActorId GetActorA() const
	{
		return m_ActorA;
	}

	ActorId GetActorB() const
	{
		return m_ActorB;
	}

	const Vec3& GetSumNormalForce() const
	{
		return m_SumNormalForce;
	}

	const Vec3& GetSumFrictionForce() const
	{
		return m_SumFrictionForce;
	}

	const Vec3List& GetCollisionPoints() const
	{
		return m_CollisionPoints;
	}

	virtual void VBuildEventData();

	EXPORT_FOR_SCRIPT_EVENT(EvtData_PhysCollision);
};


class EvtData_PhysSeparation : public BaseEventData
{
	ActorId m_ActorA;
	ActorId m_ActorB;

public:
	static const EventType s_EventType;

public:

	EvtData_PhysSeparation()
	{
		m_ActorA = INVALID_ACTOR_ID;
		m_ActorB = INVALID_ACTOR_ID;
	}

	explicit EvtData_PhysSeparation(ActorId actorA, ActorId actorB)
	{
		m_ActorA = actorA;
		m_ActorB = actorB;
	}

	virtual const EventType& VGetEventType() const
	{
		return s_EventType;
	}

	virtual const char* VGetName() const
	{
		return "EvtData_PhysSeparation";
	}

	IEventDataPtr VCopy() const
	{
		return IEventDataPtr(New EvtData_PhysSeparation(m_ActorA, m_ActorB));
	}

	ActorId GetActorA() const
	{
		return m_ActorA;
	}

	ActorId GetActorB() const
	{
		return m_ActorB;
	}

};


