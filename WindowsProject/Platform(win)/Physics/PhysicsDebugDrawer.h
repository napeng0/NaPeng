#pragma once
#include "3rdParty\Source\GCC4\3rdParty\bullet-2.79\src\btBulletDynamicsCommon.h"


class BulletDebugDrawer : public btIDebugDraw
{
	DebugDrawModes m_DebugModes;
public:
	// btIDebugDraw interfaces
	virtual void	DrawContactPoint(const btVector3& PointOnB, const btVector3& normalOnB, btScalar distance, int lifeTime, const btVector3& color) override;
	virtual void	ReportErrorWarning(const char* warningString) override;
	virtual void	SetDebugMode(int debugMode) override;
	virtual int		GetDebugMode() const override;
	virtual void	DrawLine(const btVector3& from, const btVector3& to, const btVector3& color) override;
	virtual void    Draw3dText(const btVector3& location, const char* textString) override;

	//Read PlayerOptions.xml to turn on physics debug options.
	void ReadOptions(void);
};


