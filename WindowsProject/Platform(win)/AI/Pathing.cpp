#include"GameCodeStd.h"
#include"Pathing.h"
#include"GameCode\GameCode.h"

MEMORYPOOL_AUTOINIT(PathingNode, 128);

void PathingNode::AddArc(PathingArc* pArc)
{
	ASSERT(pArc);
	m_Arcs.push_back(pArc);
}

void PathingNode::GetNeighbors(PathingNodeList& out)
{
	for (PathingArcList::iterator it = m_Arcs.begin(); it != m_Arcs.end(); ++it)
	{
		PathingArc* pArc = *it;
		out.push_back(pArc->GetNeighbor(this));
	}
}


float PathingNode::GetCostFromNode(PathingNode* pAnother)
{
	ASSERT(pAnother);
	PathingArc* pArc = FindArc(pAnother);
	ASSERT(pAnother);
	Vec3 diff = pAnother->GetPos() - m_Pos;
	return (pArc->GetWeight()* diff.Length());
}


PathingArc* PathingNode::FindArc(PathingNode* pAnother)
{
	ASSERT(pAnother);
	for (PathingArcList::iterator it = m_Arcs.begin(); it != m_Arcs.end(); ++it)
	{
		PathingArc* pArc = *it;
		if (pArc->GetNeighbor(this) == pAnother)
			return pArc;
	}
	return NULL;
}


void PathingArc::LinkNodes(PathingNode* pNodeA, PathingNode* pNodeB)
{
	ASSERT(pNodeA);
	ASSERT(pNodeB);
	m_pNode[0] = pNodeA;
	m_pNode[1] = pNodeB;
}


PathingNode* PathingArc::GetNeighbor(PathingNode* pThis)
{
	ASSERT(pThis);
	if (m_pNode[0] == pThis)
		return m_pNode[1];
	else 
		return m_pNode[0];
}


bool PathPlan::CheckForNextNode(const Vec3& pos)
{
	if (m_Index == m_Path.end())
		return false;

	Vec3 diff = pos - (*m_Index)->GetPos();
	if (diff.Length() <= (*m_Index)->GetTolerance())
	{
		++m_Index;
		return true;
	}
	return false;
}


bool PathPlan::CheckForEnd()
{
	if (m_Index == m_Path.end())
		return true;
	return false;
}


void PathPlan::AddNode(PathingNode* pNode)
{
	ASSERT(pNode);
	m_Path.push_front(pNode);
}


PathPlanNode::PathPlanNode(PathingNode* pNode, PathPlanNode* pPrev, PathingNode* pGoalNode)
{
	ASSERT(pNode);

	m_pPathingNode = pNode;
	m_pPrev = pPrev;
	m_pGoalNode = pGoalNode;
	m_IsClosed = false;
	UpdateHeuristics();
}

void PathPlanNode::UpdatePrevNode(PathPlanNode* pPrev)
{
	ASSERT(pPrev);
	m_pPrev = pPrev;
	UpdateHeuristics();
}

void PathPlanNode::UpdateHeuristics()
{
	if (m_pPrev)
		m_Goal = m_pPrev->GetGoal() + m_pPathingNode->GetCostFromNode(m_pPrev->GetPathingNode());
	else
		m_Goal = 0;

	Vec3 diff = m_pPathingNode->GetPos() - m_pGoalNode->GetPos();
	m_Heuristic = diff.Length();

	m_Fitness = m_Goal + m_Heuristic;
}


AStar::AStar()
{
	m_pStartNode = NULL;
	m_pGoalNode = NULL;
}

AStar::~AStar()
{
	Destroy();
}

void AStar::Destroy()
{
	for (PathingNodeToPathPlanNodeMap::iterator it = m_Nodes.begin(); it != m_Nodes.end(); ++it)
		delete it->second;
	m_Nodes.clear();
	m_pStartNode = NULL;
	m_pGoalNode = NULL;
}


PathPlan* AStar::operator()(PathingNode* pStartNode, PathingNode* pGoalNode)
{
	ASSERT(pStartNode);
	ASSERT(pGoalNode);

	if (pStartNode == pGoalNode)
		return NULL;

	m_pStartNode = pStartNode;
	m_pGoalNode = pGoalNode;

	AddToOpenSet(m_pStartNode, NULL);
	//m_OpenSet is a priority queue of the nodes to be evaluated
	while (!m_OpenSet.empty())
	{
		PathPlanNode* pNode = m_OpenSet.front();
		if (pNode->GetPathingNode() == m_pGoalNode)
			return RebuildPath(pNode);

		m_OpenSet.pop_front();
		AddToClosedSet(pNode);

		PathingNodeList neighbors;
		pNode->GetPathingNode()->GetNeighbors(neighbors);

		for (PathingNodeList::iterator it = neighbors.begin(); it != neighbors.end(); ++it)
		{
			PathingNode* pNodeToEvaluate = *it;
			PathingNodeToPathPlanNodeMap::iterator find = m_Nodes.find(pNodeToEvaluate);
			
			//Skip the nodes in closed list
			if (find != m_Nodes.end() && find->second->IsClosed())
				continue;

			float Cost = pNode->GetGoal() + pNodeToEvaluate->GetCostFromNode(pNode->GetPathingNode());
			bool isBetterPath = false;

			//Search for the node to be evaluated in open list
			PathPlanNode* pPathPlanNodeToEvaluate = NULL;
			if (find != m_Nodes.end())
				pPathPlanNodeToEvaluate = find->second;

			//If The node to be evaluated isn't in open list, add it into the open list
			if (!pPathPlanNodeToEvaluate)
				pPathPlanNodeToEvaluate = AddToOpenSet(pNodeToEvaluate, pNode);

			//If The Node to be evaluated is already in the open list
			//Check if this path is better
			else if (Cost < pPathPlanNodeToEvaluate->GetGoal())
				isBetterPath = true;
			//If path through the node being evaluated is better,
			//relink the nodes properly, and sort the priority queue
			if (isBetterPath)
			{
				pPathPlanNodeToEvaluate->UpdatePrevNode(pNode);
				ReinsertNode(pPathPlanNodeToEvaluate);
			}
		}
	}

	//Nodes ran out ,can't find a path
	return NULL;




}

PathPlanNode* AStar::AddToOpenSet(PathingNode* pNode, PathPlanNode* pPrev)
{
	ASSERT(pNode);
	ASSERT(pPrev);

	PathingNodeToPathPlanNodeMap::iterator it = m_Nodes.find(pNode);
	PathPlanNode* pThisNode = NULL;
	if (it == m_Nodes.end())
	{
		pThisNode = New PathPlanNode(pNode, pPrev, m_pGoalNode);
		m_Nodes.insert(std::make_pair(pNode, pThisNode));
	}
	else
	{
		WARNING("Adding existing PathPlanNode to open list");
		pThisNode = it->second;
		pThisNode->SetClosed(false);
	}

	InsertNode(pThisNode);

	return pThisNode;

}


void AStar::AddToClosedSet(PathPlanNode* pNode)
{
	ASSERT(pNode);
	pNode->SetClosed(true);
}

void AStar::InsertNode(PathPlanNode* pNode)
{
	ASSERT(pNode);
	if (m_OpenSet.empty())
	{
		m_OpenSet.push_back(pNode);
		return;
	}
	
	PathPlanNodeList::iterator it = m_OpenSet.begin();
	PathPlanNode* pCompare = *it;
	while (pCompare->IsBetterThan(pNode))
	{
		++it;
		if (it != m_OpenSet.end())
			pCompare = *it;
		else
			break;
	}
	m_OpenSet.insert(it, pNode);
}


void AStar::ReinsertNode(PathPlanNode* pNode)
{
	ASSERT(pNode);
	
	for (PathPlanNodeList::iterator it = m_OpenSet.begin(); it != m_OpenSet.end(); ++it)
	{
		if (pNode == *it)
		{
			m_OpenSet.erase(it);
			InsertNode(pNode);
			return;
		}

	}

	WARNING("Attempting to reinsert node that was never in open list");
	InsertNode(pNode);
}

PathPlan* AStar::RebuildPath(PathPlanNode* pEnd)
{
	ASSERT(pEnd);
	PathPlan* pPlan = New PathPlan;
	PathPlanNode* pNode = pEnd;
	while (pNode)
	{
		pPlan->AddNode(pNode->GetPathingNode());
		pNode = pNode->GetPrev();
	}
	return pPlan;
}

void PathingGraph::DestroyGraph()
{
	for (PathingNodeVec::iterator it = m_Nodes.begin(); it != m_Nodes.end(); ++it)
	{
		delete *it;
		m_Arcs.clear();
	}
}


PathingNode* PathingGraph::FindClosestNode(const Vec3& pos)
{
	//A brutal force algorithm check every Pathing node
	PathingNode* pClosest = m_Nodes.front();
	float length = FLT_MAX;
	for (PathingNodeVec::iterator it = m_Nodes.begin(); it != m_Nodes.end(); ++it)
	{
		PathingNode* pNode = *it;
		Vec3 diff = pos - pNode->GetPos();
		if (diff.Length() < length)
		{
			pClosest = pNode;
			length = diff.Length();
		}
	}

	return pClosest;
}


PathingNode* PathingGraph::FindFurthestNode(const Vec3& pos)
{
	//Brutal force algorithm check every node
	PathingNode* pFurthest = m_Nodes.front();
	float length = 0;
	for (PathingNodeVec::iterator it = m_Nodes.begin(); it != m_Nodes.end(); ++it)
	{
		PathingNode* pNode = *it;
		Vec3 diff = pos - pNode->GetPos();
		if (diff.Length() > length)
		{
			pFurthest = pNode;
			length = diff.Length();
		}
	}

	return pFurthest;
}


class GameCodeApp;
extern GameCodeApp* g_pApp;
PathingNode* PathingGraph::FindRandomNode()
{
	unsigned int numNodes = (unsigned int)m_Nodes.size();
	unsigned int node = g_pApp->m_pGame->GetRandomGenerator().GetRandom(numNodes);

	//If randomly selected node is in the first half, search from beginning
	if (node <= numNodes / 2)
	{
		PathingNodeVec::iterator it = m_Nodes.begin();
		for (unsigned int i = 0; i < node; ++i)
			++it;
		return *it;
	}
	//otherwise, start from the end
	else
	{
		PathingNodeVec::iterator it = m_Nodes.end();
		for (unsigned int i = numNodes; i > node; --i)
			--it;
		return *it;
	}
}


PathPlan* PathingGraph::FindPath(const Vec3& start, const Vec3& end)
{
	PathingNode* pStartNode = FindClosestNode(start);
	PathingNode* pGoalNode = FindClosestNode(end);
	return FindPath(pStartNode, pGoalNode);
}

PathPlan* PathingGraph::FindPath(PathingNode* pStartNode, const Vec3& end)
{
	PathingNode* pGoalNode = FindClosestNode(end);
	return FindPath(pStartNode, pGoalNode);
}

PathPlan* PathingGraph::FindPath(const Vec3& start, PathingNode* pGoalNode)
{
	PathingNode* pStartNode = FindClosestNode(start);
	return FindPath(pStartNode, pGoalNode);
}

PathPlan* PathingGraph::FindPath(PathingNode* pStartNode, PathingNode* pGoalNode)
{
	AStar aStar;
	return aStar(pStartNode, pGoalNode);
}


void PathingGraph::BuildTestGraph()
{
	if (!m_Nodes.empty())
		DestroyGraph();
	
	m_Nodes.reserve(128);

	int index = 0;
	for (float x = -45.0f; x < 45.0f; x += 10.0f)
	{
		for (float z = -45.0f; z < 45.0f; z += 10.0f)
		{
			PathingNode* pNode = new PathingNode(Vec3(x, 0, z));
			m_Nodes.push_back(pNode);

			int temp = index - 1;
			if (temp >= 0)
				LinkNodes(m_Nodes[temp], pNode);

			++index;
		}
	}

}


void PathingGraph::LinkNodes(PathingNode* pNodeA, PathingNode* pNodeB)
{
	ASSERT(pNodeA);
	ASSERT(pNodeB);

	PathingArc* pArc = New PathingArc;
	pArc->LinkNodes(pNodeA, pNodeB);
	pNodeA->AddArc(pArc);
	pNodeB->AddArc(pArc);
	m_Arcs.push_back(pArc);
}

PathingGraph* CreatePathingGraph()
{
	PathingGraph* pPathingGraph = New PathingGraph;
	pPathingGraph->BuildTestGraph();
	return pPathingGraph;
}
