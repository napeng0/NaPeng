#include"GameCodeStd.h"
#include"PhysicsDebugDrawer.h"
#include"UI\HumanView.h"
#include"GameCode\GameCode.h"


void BulletDebugDrawer::DrawContactPoint(const btVector3& PointOnB, const btVector3& normalOnB, btScalar distance, int lifeTime, const btVector3& color)
{

	const btVector3 startPoint = PointOnB;
	const btVector3 endPoint = PointOnB + normalOnB * distance;

	DrawLine(startPoint, endPoint, color);
}


void BulletDebugDrawer::ReportErrorWarning(const char* warningString)
{
	WARNING(warningString);
}

void BulletDebugDrawer::SetDebugMode(int debugMode)
{
	m_DebugModes = (DebugDrawModes)debugMode;
}


int BulletDebugDrawer::GetDebugMode() const
{
	return m_DebugModes;
}


void BulletDebugDrawer::ReadOptions()
{
	TiXmlDocument* optionsDoc = g_pApp->m_Options.m_pDoc;
	TiXmlElement* pRoot = optionsDoc->RootElement();
	if (!pRoot)
		return;

	int debugModes = btIDebugDraw::DBG_NoDebug;
	TiXmlElement* pNode = pRoot->FirstChildElement("PhysicsDebug");
	if (pNode)
	{
		if (pNode->Attribute("DrawWireFrame"))
		{
			std::string attribute(pNode->Attribute("DrawWireFrame"));
			if (attribute == "yes") 
				debugModes |= btIDebugDraw::DBG_DrawWireframe;
		}

		if (pNode->Attribute("DrawAabb"))
		{
			std::string attribute(pNode->Attribute("DrawAabb"));
			if (attribute == "yes") 
				debugModes |= btIDebugDraw::DBG_DrawAabb;
		}

		if (pNode->Attribute("DrawFeaturesText"))
		{
			std::string attribute(pNode->Attribute("DrawFeaturesText"));
			if (attribute == "yes") 
				debugModes |= btIDebugDraw::DBG_DrawFeaturesText;
		}

		if (pNode->Attribute("DrawContactPoints"))
		{
			std::string attribute(pNode->Attribute("DrawContactPoints"));
			if (attribute == "yes") 
				debugModes |= btIDebugDraw::DBG_DrawContactPoints;
		}

		if (pNode->Attribute("NoDeactivation"))
		{
			std::string attribute(pNode->Attribute("NoDeactivation"));
			if (attribute == "yes") 
				debugModes |= btIDebugDraw::DBG_NoDeactivation;
		}

		if (pNode->Attribute("NoHelpText"))
		{
			std::string attribute(pNode->Attribute("NoHelpText"));
			if (attribute == "yes") 
				debugModes |= btIDebugDraw::DBG_NoHelpText;
		}

		if (pNode->Attribute("DrawText"))
		{
			std::string attribute(pNode->Attribute("DrawText"));
			if (attribute == "yes") 
				debugModes |= btIDebugDraw::DBG_DrawText;
		}

		if (pNode->Attribute("ProfileTimings"))
		{
			std::string attribute(pNode->Attribute("ProfileTimings"));
			if (attribute == "yes") 
				debugModes |= btIDebugDraw::DBG_ProfileTimings;
		}

		if (pNode->Attribute("EnableSatComparison"))
		{
			std::string attribute(pNode->Attribute("EnableSatComparison"));
			if (attribute == "yes") 
				debugModes |= btIDebugDraw::DBG_EnableSatComparison;
		}

		if (pNode->Attribute("DisableBulletLCP"))
		{
			std::string attribute(pNode->Attribute("DisableBulletLCP"));
			if (attribute == "yes") 
				debugModes |= btIDebugDraw::DBG_DisableBulletLCP;
		}

		if (pNode->Attribute("EnableCCD"))
		{
			std::string attribute(pNode->Attribute("EnableCCD"));
			if (attribute == "yes") 
				debugModes |= btIDebugDraw::DBG_EnableCCD;
		}

		if (pNode->Attribute("DrawConstraints"))
		{
			std::string attribute(pNode->Attribute("DrawConstraints"));
			if (attribute == "yes") 
				debugModes |= btIDebugDraw::DBG_DrawConstraints;
		}

		if (pNode->Attribute("DrawConstraintLimits"))
		{
			std::string attribute(pNode->Attribute("DrawConstraintLimits"));
			if (attribute == "yes") 
				debugModes |= btIDebugDraw::DBG_DrawConstraintLimits;
		}

		if (pNode->Attribute("FastWireframe"))
		{
			std::string attribute(pNode->Attribute("FastWireframe"));
			if (attribute == "yes") 
				debugModes |= btIDebugDraw::DBG_FastWireframe;
		}

		if (debugModes != btIDebugDraw::DBG_NoDebug)
		{
			SetDebugMode(debugModes);
		}
	}

}

void BulletDebugDrawer::DrawLine(const btVector3& from, const btVector3& to, const btVector3& lineColor)
{
	shared_ptr<Scene> pScene = g_pApp->GetHumanView()->m_pScene;
	shared_ptr<IRenderer> pRenderer = pScene->GetRenderer();

	Vec3 vec3From, vec3To;
	vec3From.x = from.x();
	vec3From.y = from.y();
	vec3From.z = from.z();

	vec3To.x = to.x();
	vec3To.y = to.y();
	vec3To.z = to.z();

	Color color = D3DCOLOR_XRGB(BYTE(255 * lineColor.x()), BYTE(255 * lineColor.y()), BYTE(255 * lineColor.z()));

	pRenderer->VDrawLine(vec3From, vec3To, color);

}

void BulletDebugDrawer::Draw3dText(const btVector3 & location, const char * textString)
{
}


