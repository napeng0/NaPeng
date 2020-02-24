#pragma once
#include"GameCodeStd.h"
#include"Memory\MemoryPool.h"
#include"Memory\MemoryMacros.h"

class PathingArc;
class PathingNode;
class PathPlanNode;
class AStar;

typedef std::list<PathingArc*> PathingArcList;
typedef std::list<PathingNode*> PathingNodeList;
typedef std::vector<PathingNode*> PathingNodeVec;
typedef std::map<PathingNode*, PathPlanNode*> PathingNodeToPathPlanNodeMap;
typedef std::list<PathPlanNode*> PathPlanNodeList;

const float PATHING_DEFAULT_NODE_TOLERANCE = 5.0f;
const float PATHING_DEFAULT_ARC_WEIGHT = 1.0f;

class PathingNode
{
	float m_Tolerance;
	Vec3 m_Pos;
	PathingArcList m_Arcs;

public:
	explicit PathingNode(const Vec3& pos, float tolerance = PATHING_DEFAULT_NODE_TOLERANCE)
	{
		m_Pos = pos;
		m_Tolerance = tolerance;
	}
	const Vec3& GetPos() const { return m_Pos; }
	float GetTolerance() const { return m_Tolerance; }
	void AddArc(PathingArc* pArc);
	void GetNeighbors(PathingNodeList& neighbors);
	float GetCostFromNode(PathingNode* pNode);//Only adjacent nodes

private:
	PathingArc* FindArc(PathingNode* pLinkedNode);
	MEMORYPOOL_DECLARATION(0);

};
MEMORYPOOL_DEFINITION(PathingNode);

class PathingArc
{
	float m_Weight;
	PathingNode* m_pNode[2];

public:
	explicit PathingArc(float weight = PATHING_DEFAULT_ARC_WEIGHT) { m_Weight = weight; }
	float GetWeight() const { return m_Weight; }
	void LinkNodes(PathingNode* pNodeA, PathingNode* pNodeB);
	PathingNode* GetNeighbor(PathingNode* pNodeA);
};


//This class represents a complete path
class PathPlan
{
	friend class AStar;

	PathingNodeList m_Path;
	PathingNodeList::iterator m_Index;

public:
	PathPlan() { m_Index = m_Path.end(); }
	void ResetPath() { m_Index = m_Path.begin(); }
	const Vec3& GetCurrentNodePosition() const 
	{
		ASSERT(m_Index != m_Path.end());
		return (*m_Index)->GetPos();
	}
	bool CheckForNextNode(const Vec3& pos);
	bool CheckForEnd();

private:
	void AddNode(PathingNode* pNode);

};


//Pathing node but with extra information for pathing
class PathPlanNode
{
	PathPlanNode* m_pPrev;//Parent node
	PathingNode* m_pPathingNode;//This node
	PathingNode* m_pGoalNode;
	bool m_IsClosed;
	float m_Goal; //Cost of the path up to this point(The G value)
	float m_Heuristic; //Estimated cost of this node to the goal(The H value)
	float m_Fitness; //Estimated cost from start to goal through this node(The F value)

public:
	explicit PathPlanNode(PathingNode* pNode, PathPlanNode* pPrev, PathingNode* pGoal);
	PathPlanNode* GetPrev() const { return m_pPrev; }
	PathingNode* GetPathingNode() const { return m_pPathingNode; }
	bool IsClosed()const { return m_IsClosed; }
	float GetGoal() const { return m_Goal; }
	float GetHeuristc() const { return m_Heuristic; }
	float GetFitness() const { return m_Fitness; }
	void UpdatePrevNode(PathPlanNode* pPrev);
	void SetClosed(bool isClosed = true) { m_IsClosed = isClosed; }
	bool IsBetterThan(PathPlanNode* pAnother) { return (m_Fitness < pAnother->GetFitness()); }

private:
	void UpdateHeuristics();

};


class AStar
{
	PathingNodeToPathPlanNodeMap m_Nodes;
	PathingNode* m_pStartNode;
	PathingNode* m_pGoalNode;
	PathPlanNodeList m_OpenSet;

public:
	AStar();
	~AStar();

	void Destroy();

	PathPlan* operator()(PathingNode* pStartNode, PathingNode* PGoalNode);

private:
	PathPlanNode* AddToOpenSet(PathingNode* pNode, PathPlanNode* pPrev);
	void AddToClosedSet(PathPlanNode* pNode);
	void InsertNode(PathPlanNode* pNode);
	void ReinsertNode(PathPlanNode* pNode);
	PathPlan* RebuildPath(PathPlanNode* pGoalNode);
};


class PathingGraph
{
	PathingNodeVec m_Nodes;
	PathingArcList m_Arcs;

public:
	PathingGraph() {}
	~PathingGraph() { DestroyGraph(); }

	void DestroyGraph();

	PathingNode* FindClosestNode(const Vec3& pos);
	PathingNode* FindFurthestNode(const Vec3& pos);
	PathingNode* FindRandomNode();
	PathPlan* FindPath(const Vec3& start, const Vec3& end);
	PathPlan* FindPath(const Vec3& start, PathingNode* pGoalNode);
	PathPlan* FindPath(PathingNode* pStartNode, PathingNode* pGoalNode);
	PathPlan* FindPath(PathingNode* pStartNode, const Vec3& end);

	void BuildTestGraph();

private:
	void LinkNodes(PathingNode* pNodeA, PathingNode* pNodeB);
};

PathingGraph* CreatePathingGraph();
