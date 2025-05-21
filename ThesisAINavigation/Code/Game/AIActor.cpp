#include "AIActor.hpp"
#include "Game/Actor.hpp"
#include "Game/PlayerActor.hpp"
#include "Game/Map.hpp"
#include "Game/GameCommon.hpp"
#include "Game/ActorDefinitions.hpp"
#include "Engine/AI/Pathfinding/NavMeshPathfinding.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Core/Timer.hpp"
#include "Engine/Core/DebugDraw.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/Time.hpp"
#include "Engine/Core/DevConsole.hpp"
#include "Engine/Math/Mat44.hpp"
#include "Engine/Math/RaycastUtils.hpp"
#include "Engine/Math/LineSegment3.hpp"
#include "Engine/Math/RaycastUtils.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Renderer/NavMesh.hpp"
#include "Engine/Renderer/DebugRenderer.hpp"
#include <map>
#include <cmath>

AIActor::AIActor(Game* game, Map* map, NavMesh* navMesh, NavMeshPathfinding* path)
	: m_currentGame(game), m_currentMap(map), m_currentNavMesh(navMesh), m_currentPath(path)
{
	m_obstacleAvoidance = new ObstacleAvoidnace(m_currentNavMesh, m_currentNavMesh->m_heatMap);
	g_rng.SetSeed(static_cast<unsigned int>(std::chrono::high_resolution_clock::now().time_since_epoch().count()));
	m_repathDuration = g_rng.SRollRandomFloatInRange(1.5f, 2.5f);
	m_repathTimeRemaining = m_repathDuration;
}

AIActor::AIActor(Game* game, PlayGround* playGround)
	: m_currentGame(game), m_currentPlayGround(playGround)
{
	m_obstacleAvoidance = new ObstacleAvoidnace();
}

void AIActor::Update()
{
	if (m_currentMap)
	{
		m_actor = m_currentMap->GetActorByUID(m_actorUID);
	}
	else if (m_currentPlayGround)
	{
		m_actor = m_currentPlayGround->GetActorByUID(m_actorUID);
	}

	if (m_currentGame->m_gameModeConfig.m_useAStar || m_currentGame->m_gameModeConfig.m_useAStar && m_currentGame->m_gameModeConfig.m_useORCA)
	{
		AiTraversalUpdate(GetActor()->m_position);
		
		std::vector<Job*> completedJobs;
		g_theJobSystem->RetrieveCompletedJobs(completedJobs, 20);
		for (Job* completedPathingJob : completedJobs)
		{
			AStarPathfindingJob* pathingJob = dynamic_cast<AStarPathfindingJob*>(completedPathingJob);
			if (!pathingJob)
			{
				delete completedPathingJob;
				continue;
			}

			pathingJob->m_ai->m_aiPath = pathingJob->GetResult();
			pathingJob->m_ai->m_isWaitingForPath = false;
			if (pathingJob->m_ai->m_aiPath.empty())
			{
				pathingJob->m_ai->m_hasReachedGoal = true;          // Trigger new goal generation
				pathingJob->m_ai->m_repathTimeRemaining = -1.f;     // Force immediate retry
			}
			delete pathingJob;
		}

		MoveAlongPathUpdate();
	}
	else if (!m_currentGame->m_gameModeConfig.m_useAStar)
	{
		CurrentObstacleMode();
	}
}

FOVZone AIActor::GetFOVZone(Vec3 const& selfPosition, Vec3 const& fwdDir, Vec3 const& otherPosition)
{
	Vec3 toOther = (otherPosition - selfPosition).GetNormalized();
	Vec3 forwardNormalized = fwdDir.GetNormalized();

	float angleDegrees = GetAngleDegreesBetweenVectors3D(forwardNormalized, toOther);

	if (angleDegrees <= 40.f) return FOVZone::CENTRAL;
	if (angleDegrees <= 100.f) return FOVZone::MID_PERIPHERY;
	if (angleDegrees <= 130.f) return FOVZone::FAR_PERPHERY;
	return FOVZone::BEHIND;
}

void AIActor::AgentFOV(Vec3 const& originPos, Vec3 const& forwardDir, float angleRadius)
{
	const float halfFOV = FOV_DEGREES * 0.5f;
	const float angleStep = FOV_DEGREES / static_cast<float>(NUM_CONE_SEGMENTS);

	std::vector<Vec3> conePoints;
	conePoints.reserve(NUM_CONE_SEGMENTS + 1);
	conePoints.emplace_back(originPos + forwardDir * angleRadius);

	for (int i = 0; i <= NUM_CONE_SEGMENTS; i++)
	{
		float angle = -halfFOV + i * angleStep;
		Vec3 rotated = forwardDir.GetRotatedAboutZDegrees(angle).GetNormalized();
		Vec3 point = originPos + rotated * angleRadius;
		conePoints.emplace_back(point);
	}

	// Draw wireframe triangles for FOV cone
	if (m_currentGame->m_enableFOVZoneVisual)
	{
		for (int i = 1; i < static_cast<int>(conePoints.size()) - 1; i++)
		{
			DebugAddWorld3DWireTriangle(originPos, conePoints[i], conePoints[i + 1], 0.f, Rgba8::YELLOW, Rgba8::YELLOW, DebugRenderMode::ALWAYS);
		}
	}
}

void AIActor::ApplyFinalMovement(Vec3 const& finalDirection, float maxTurnAngle, float maxAngleBeforeApplyingMovement)
{
	if (finalDirection.IsNearlyZero()) return;

	float turnTowards = finalDirection.GetAngleAboutZDegrees();
	m_actor->TurnInDirection(turnTowards, maxTurnAngle);

	float angleDiff = fabsf(GetShortestAngularDispDegrees(m_actor->m_orientation.m_yawDegrees, turnTowards));
	if (angleDiff <= maxAngleBeforeApplyingMovement)
	{
		float moveSpeedScale = RangeMapClamped(angleDiff, 0.f, 90.f, 1.f, 0.f);
		m_actor->MoveInDirection(finalDirection, m_actor->m_moveSpeed * moveSpeedScale);
	}
}

void AIActor::CurrentObstacleMode()
{
	if (m_currentGame->m_gameModeConfig.m_useVO)
	{
		Vec3 direction;
		std::vector<Actor*> nearbyAgents;
		GetNearbyAgentsOnThePlayGround(nearbyAgents, m_actor->m_position, m_actor->m_searchRadius);

		if (!nearbyAgents.empty())
		{
			direction = m_actor->m_orientation.GetForwardVector();
			m_actor->m_preferredVelocity = direction * m_actor->m_moveSpeed;

			m_actor->UpdateAIAgent();

			std::vector<AIAgent*> nearbyAI;
			nearbyAI.reserve(nearbyAgents.size());

			for (Actor* other : nearbyAgents)
			{
				other->UpdateAIAgent(); // Sync each nearby actor's agent
				nearbyAI.emplace_back(&other->m_agent);
			}

			m_obstacleAvoidance->ComputeVO(m_actor->m_agent, m_actor->m_searchRadius, nearbyAI, m_currentGame->m_enableCurrentVOAlgorithmVisual);
			m_actor->m_velocity = m_actor->m_agent.m_velocity;

			Vec3 adjustedDirection = m_actor->m_velocity.GetNormalized();
			float turnTowards = adjustedDirection.GetAngleAboutZDegrees();
			float maxTurnAngle = m_actor->m_turnSpeed * m_actor->m_moveSpeed * m_currentGame->m_clock->GetDeltaSeconds();

			m_actor->TurnInDirection(turnTowards, maxTurnAngle);
			float angleDiff = fabsf(GetShortestAngularDispDegrees(m_actor->m_orientation.m_yawDegrees, turnTowards));
			float moveSpeedScale = RangeMapClamped(angleDiff, 0.f, 90.f, 1.f, 0.f);
			m_actor->MoveInDirection(adjustedDirection, m_actor->m_moveSpeed * moveSpeedScale);
		}
		else
		{
			direction = m_actor->m_initialDirection;
			m_actor->m_preferredVelocity = direction * m_actor->m_moveSpeed;

			float turnTowards = direction.GetAngleAboutZDegrees();
			float maxTurnAngle = m_actor->m_turnSpeed * m_currentGame->m_clock->GetDeltaSeconds();

			m_actor->TurnInDirection(turnTowards, maxTurnAngle);
			float angleDiff = fabsf(GetShortestAngularDispDegrees(m_actor->m_orientation.m_yawDegrees, turnTowards));
			float moveSpeedScale = RangeMapClamped(angleDiff, 0.f, 90.f, 1.f, 0.f);
			m_actor->MoveInDirection(direction, m_actor->m_moveSpeed * moveSpeedScale);
		}
	}
	else if (m_currentGame->m_gameModeConfig.m_useRVO)
	{
		Vec3 direction;
		std::vector<Actor*> nearbyAgents;
		GetNearbyAgentsOnThePlayGround(nearbyAgents, m_actor->m_position, m_actor->m_searchRadius);

		if (!nearbyAgents.empty())
		{
			direction = m_actor->m_orientation.GetForwardVector();
			m_actor->m_preferredVelocity = direction * m_actor->m_moveSpeed;

			m_actor->UpdateAIAgent();

			std::vector<AIAgent*> nearbyAI;
			nearbyAI.reserve(nearbyAgents.size());

			for (Actor* other : nearbyAgents)
			{
				other->UpdateAIAgent(); // Sync each nearby actor's agent
				nearbyAI.emplace_back(&other->m_agent);
			}

			m_obstacleAvoidance->ComputeRVO(m_actor->m_agent, m_actor->m_searchRadius, nearbyAI, m_currentGame->m_enableCurrentVOAlgorithmVisual);
			m_actor->m_velocity = m_actor->m_agent.m_velocity;

			Vec3 adjustedDirection = m_actor->m_velocity.GetNormalized();
			float turnTowards = adjustedDirection.GetAngleAboutZDegrees();
			float maxTurnAngle = m_actor->m_turnSpeed * m_actor->m_moveSpeed * m_currentGame->m_clock->GetDeltaSeconds();

			m_actor->TurnInDirection(turnTowards, maxTurnAngle);
			float angleDiff = fabsf(GetShortestAngularDispDegrees(m_actor->m_orientation.m_yawDegrees, turnTowards));
			float moveSpeedScale = RangeMapClamped(angleDiff, 0.f, 90.f, 1.f, 0.f);
			m_actor->MoveInDirection(adjustedDirection, m_actor->m_moveSpeed * moveSpeedScale);
		}
		else
		{
			direction = m_actor->m_initialDirection;
			m_actor->m_preferredVelocity = direction * m_actor->m_moveSpeed;

			float turnTowards = direction.GetAngleAboutZDegrees();
			float maxTurnAngle = m_actor->m_turnSpeed * m_currentGame->m_clock->GetDeltaSeconds();

			m_actor->TurnInDirection(turnTowards, maxTurnAngle);
			float angleDiff = fabsf(GetShortestAngularDispDegrees(m_actor->m_orientation.m_yawDegrees, turnTowards));
			float moveSpeedScale = RangeMapClamped(angleDiff, 0.f, 90.f, 1.f, 0.f);
			m_actor->MoveInDirection(direction, m_actor->m_moveSpeed * moveSpeedScale);
		}
	}
	else if (m_currentGame->m_gameModeConfig.m_useHRVO)
	{
		Vec3 direction;
		std::vector<Actor*> nearbyAgents;
		GetNearbyAgentsOnThePlayGround(nearbyAgents, m_actor->m_position, m_actor->m_searchRadius);

		if (!nearbyAgents.empty())
		{
			direction = m_actor->m_orientation.GetForwardVector();
			m_actor->m_preferredVelocity = direction * m_actor->m_moveSpeed;

			m_actor->UpdateAIAgent();

			std::vector<AIAgent*> nearbyAI;
			nearbyAI.reserve(nearbyAgents.size());

			for (Actor* other : nearbyAgents)
			{
				other->UpdateAIAgent(); // Sync each nearby actor's agent
				nearbyAI.emplace_back(&other->m_agent);
			}

			m_obstacleAvoidance->ComputeHRVO(m_actor->m_agent, m_actor->m_searchRadius, nearbyAI, m_currentGame->m_enableCurrentVOAlgorithmVisual);
			m_actor->m_velocity = m_actor->m_agent.m_velocity;

			Vec3 adjustedDirection = m_actor->m_velocity.GetNormalized();
			float turnTowards = adjustedDirection.GetAngleAboutZDegrees();
			float maxTurnAngle = m_actor->m_turnSpeed * m_actor->m_moveSpeed * m_currentGame->m_clock->GetDeltaSeconds();

			m_actor->TurnInDirection(turnTowards, maxTurnAngle);
			float angleDiff = fabsf(GetShortestAngularDispDegrees(m_actor->m_orientation.m_yawDegrees, turnTowards));
			float moveSpeedScale = RangeMapClamped(angleDiff, 0.f, 90.f, 1.f, 0.f);
			m_actor->MoveInDirection(adjustedDirection, m_actor->m_moveSpeed* moveSpeedScale);
		}
		else
		{
			direction = m_actor->m_initialDirection;
			m_actor->m_preferredVelocity = direction * m_actor->m_moveSpeed;

			float turnTowards = direction.GetAngleAboutZDegrees();
			float maxTurnAngle = m_actor->m_turnSpeed * m_currentGame->m_clock->GetDeltaSeconds();

			m_actor->TurnInDirection(turnTowards, maxTurnAngle);
			float angleDiff = fabsf(GetShortestAngularDispDegrees(m_actor->m_orientation.m_yawDegrees, turnTowards));
			float moveSpeedScale = RangeMapClamped(angleDiff, 0.f, 90.f, 1.f, 0.f);
			m_actor->MoveInDirection(direction, m_actor->m_moveSpeed * moveSpeedScale);
		}
	}
	else if (m_currentGame->m_gameModeConfig.m_useORCA)
	{
		Vec3 direction;
		std::vector<Actor*> nearbyAgents;
		GetNearbyAgentsOnThePlayGround(nearbyAgents, m_actor->m_position, m_actor->m_searchRadius);

		if (!nearbyAgents.empty())
		{
			direction = m_actor->m_orientation.GetForwardVector();
			m_actor->m_preferredVelocity = direction * m_actor->m_moveSpeed;

			// Ensure "agent" is moving if interacting with a static "other"
			for (Actor* other : nearbyAgents)
			{
				if (m_actor->m_velocity.GetLengthSquared() <= 0.f && other->m_velocity.GetLengthSquared() > 0.f)
				{
					// Swap roles: Treat "other" as the agent and "m_actor" as the static other
					std::swap(m_actor, other);
					break;
				}
			}

			m_actor->UpdateAIAgent();

			std::vector<AIAgent*> nearbyAI;
			nearbyAI.reserve(nearbyAgents.size());

			for (Actor* other : nearbyAgents)
			{
				other->UpdateAIAgent();
				nearbyAI.emplace_back(&other->m_agent);
			}

			m_obstacleAvoidance->ComputeORCA(m_actor->m_agent, m_actor->m_searchRadius, nearbyAI, m_currentGame->m_enableCurrentVOAlgorithmVisual);
			Vec3 finalDirection = m_actor->m_agent.m_velocity;
			float maxTurnAngle = 180.f * m_actor->m_moveSpeed * m_currentGame->m_clock->GetDeltaSeconds();
			ApplyFinalMovement(finalDirection, maxTurnAngle, 15.f);
		}
		else
		{
			direction = m_actor->m_initialDirection;
			m_actor->m_preferredVelocity = direction * m_actor->m_moveSpeed;

			float turnTowards = direction.GetAngleAboutZDegrees();
			float maxTurnAngle = m_actor->m_turnSpeed * m_currentGame->m_clock->GetDeltaSeconds();

			m_actor->TurnInDirection(turnTowards, maxTurnAngle);
			float angleDiff = fabsf(GetShortestAngularDispDegrees(m_actor->m_orientation.m_yawDegrees, turnTowards));
			float moveSpeedScale = RangeMapClamped(angleDiff, 0.f, 90.f, 1.f, 0.f);
			m_actor->MoveInDirection(direction, m_actor->m_moveSpeed* moveSpeedScale);
		}
	}
}

void AIActor::GetNearbyAgentsOnTheMap(std::vector<Actor*>& agents, const Vec3& position, float radius)
{
	if (m_currentGame->m_gameModeConfig.m_useORCA && m_currentGame->m_gameModeConfig.m_useAStar)
	{
		float radiusSq = radius * radius;

		for (Actor* agent : m_currentMap->GetAllAgents())
		{
			if (agent == nullptr || agent == m_actor) continue; // Skip if null or self

			Vec3 fwdDir = m_actor->m_orientation.GetForwardVector();
			Vec3 originPos = Vec3(m_actor->m_position.x, m_actor->m_position.y, m_actor->m_position.z + m_actor->m_eyeHeight);

			AgentFOV(originPos, fwdDir, radiusSq);

			Vec3 toOtherAgent = agent->m_position - originPos;
			toOtherAgent.z = 0.f;
			toOtherAgent.Normalize();

			float d = DotProduct3D(fwdDir, toOtherAgent);
			float fovCosThreshold = CosDegrees(FOV_DEGREES * 0.5f);
			if (d < fovCosThreshold) { continue; }
			float distacneSq = GetDistanceSquared3D(position, agent->m_position);
			if (distacneSq <= radiusSq)
			{
				agents.emplace_back(agent);
			}
		}
	}
}
 
void AIActor::GetNearbyAgentsOnThePlayGround(std::vector<Actor*>& agents, const Vec3& position, float radius)
{
	if (!m_currentGame->m_gameModeConfig.m_useAStar)
	{
		float radiusSq = radius * radius;

		for (Actor* agent : m_currentPlayGround->GetAllAgents())
		{
			if (agent == nullptr || agent == m_actor) continue;

			float distacneSq = GetDistanceSquared3D(position, agent->m_position);
			if (distacneSq <= radiusSq)
			{
				agents.emplace_back(agent);
			}
		}
	}
}

void AIActor::AiTraversalUpdate(Vec3 currentPos)
{
	if (m_hasReachedGoal && m_repathTimeRemaining <= 0.f)
	{
		// Get random goal triangle position within nav mesh 
		int randomTriangleIndex = m_currentNavMesh->GetRandomNavMeshTriangleIndex();

		m_goalPoint = m_currentPath->GetRandomPointWithinTriangleIndex(randomTriangleIndex); // Don't know why this does not work

		if (!m_currentNavMesh->GetContainingTriangleIndex(m_goalPoint))
		{
			int fallBackTriangleIndex = m_currentNavMesh->GetRandomNavMeshTriangleIndex();
			m_goalPoint = m_currentPath->GetRandomPointWithinTriangleIndex(fallBackTriangleIndex);
		}

		RequestPathfindingJob(currentPos, m_goalPoint);

		// Reset timer
		g_rng.SetSeed(static_cast<unsigned int>(std::chrono::high_resolution_clock::now().time_since_epoch().count()));
		m_repathDuration = g_rng.SRollRandomFloatInRange(1.5f, 2.5f);
		m_repathTimeRemaining = m_repathDuration;
		
		// Set goal as not yet reached
		m_hasReachedGoal = false;
	}

	// Calculate the position within the goal point & check if we have reached the goal
	float distanceSqToGoalPoint = GetDistanceSquared3D(GetActor()->m_position, m_goalPoint);
	float actorRadiusSq = GetActor()->m_physicsRadius * GetActor()->m_physicsRadius;

	if (distanceSqToGoalPoint <= actorRadiusSq * 1.1f) // Initial check for floating point errors that can occur 
	{
		m_hasReachedGoal = true;
		m_goalCheckTimer = 0.f;
	}

	// If goal is reached, tick down timer until we find a new goal
	if (m_hasReachedGoal)
	{
		m_repathTimeRemaining -= m_currentGame->m_clock->GetDeltaSeconds();
	}
	else if (!m_hasReachedGoal && m_aiPath.empty())
	{
		// Fail-safe: no path and haven't reached goal = maybe we got stuck?
		m_goalCheckTimer += m_currentGame->m_clock->GetDeltaSeconds();
		if (m_goalCheckTimer >= 0.5f)
		{
			m_hasReachedGoal = true;
			m_goalCheckTimer = 0.f;
			m_repathTimeRemaining = -1.f;
		}
	}
	else
	{
		m_goalCheckTimer = 0.f;
	}
}

void AIActor::MoveAlongPathUpdate()
{
	if (!m_aiPath.empty())
	{
		Vec3 nextPoint = m_aiPath.back();

		// Calculate direction from the current position to the center of the next mesh (triangle)
		Vec3 directionToPoint = nextPoint - m_actor->m_position;

		// Find nearby agents
		std::vector<Actor*> nearbyAgents;
		GetNearbyAgentsOnTheMap(nearbyAgents, m_actor->m_position, m_actor->m_searchRadius);

		Vec3 finalDirection = directionToPoint.GetNormalized() * m_actor->m_moveSpeed;
		float maxTurnAngle = 0.f;

		if (m_currentGame->m_gameModeConfig.m_useORCA && m_currentGame->m_gameModeConfig.m_useAStar)
		{
			if (!nearbyAgents.empty())
			{
				// Set preferred Velocity
				m_actor->m_preferredVelocity = directionToPoint * m_actor->m_moveSpeed;

				// Ensure "agent" is moving if interacting with a static "other"
				for (Actor* other : nearbyAgents)
				{
					if (m_actor->m_velocity.GetLengthSquared() <= 0.f && other->m_velocity.GetLengthSquared() > 0.f)
					{
						// Swap roles: Treat "other" as the agent and "m_actor" as the static other
						std::swap(m_actor, other);
						break;
					}
				}

				m_actor->UpdateAIAgent();

				std::vector<AIAgent*> nearbyAI;
				AgentPrioritization(nearbyAgents, nearbyAI);

				//float angleDiff = fabsf(GetShortestAngularDispDegrees(m_actor->m_orientation.m_yawDegrees, finalDirection.GetAngleAboutZDegrees()));
				m_obstacleAvoidance->ComputeORCA(m_actor->m_agent, m_actor->m_searchRadius, nearbyAI, m_currentGame->m_enableCurrentVOAlgorithmVisual);
				m_actor->m_velocity = m_actor->m_agent.m_velocity;
				//m_actor->m_position.z =  m_currentPath->GetHeightOnTriangle(m_actor->m_position);
				finalDirection = m_actor->m_velocity;
				maxTurnAngle = /*m_actor->m_turnSpeed*/ 180.f * m_actor->m_moveSpeed * m_currentGame->m_clock->GetDeltaSeconds();
				ApplyFinalMovement(finalDirection, maxTurnAngle, MAX_ANGLE_BEFORE_MOVEMENT);
			}
			else
			{
				maxTurnAngle = 180.f * m_currentGame->m_clock->GetDeltaSeconds();
				ApplyFinalMovement(finalDirection, maxTurnAngle, MAX_ANGLE_BEFORE_MOVEMENT);
			}
		}
		else if (!m_currentGame->m_gameModeConfig.m_useORCA && m_currentGame->m_gameModeConfig.m_useAStar)
		{
			maxTurnAngle = 180.f * m_currentGame->m_clock->GetDeltaSeconds();
			ApplyFinalMovement(finalDirection, maxTurnAngle, MAX_ANGLE_BEFORE_MOVEMENT);
		}

		if (m_actor->m_position.z > ALLOWED_HEIGHT_DEVIATION)
		{
			m_actor->m_position.z = Interpolate(m_actor->m_position.z, m_currentPath->GetHeightOnTriangle(m_actor->m_position), 0.3f);
		}

		// Determine if the agent should move to the next waypoint in the path
		float distanceSqToNextWaypoint = GetDistanceSquared3D(m_actor->m_position, nextPoint);
		float actorRadiusSq = m_actor->m_physicsRadius * m_actor->m_physicsRadius;

		// Skip current waypoint if effectively bypassed after ORCA adjustment
		if (distanceSqToNextWaypoint <= actorRadiusSq)
		{
			m_aiPath.pop_back(); // Reached the next waypoint, pop and continue along the path
		}
		else
		{
			// Check if the agent has moved past the waypoint after ORCA adjustment
			Vec3 directionToNext = (nextPoint - m_actor->m_previousPosition).GetNormalized();
			float dotProduct = DotProduct3D(directionToNext, finalDirection.GetNormalized());

			if (dotProduct < 0.f) // Agent has passed the target point
			{
				m_aiPath.pop_back(); // Move to the next point if we have effectively passed it
			}
		}
	}
}

void AIActor::AgentPrioritization(std::vector<Actor*>& nearbyAgents, std::vector<AIAgent*>& nearbyAI)
{
	nearbyAI.reserve(nearbyAgents.size());

	std::vector<std::pair<Actor*, FOVZone>> prioritizeAgents;

	for (Actor* other : nearbyAgents)
	{
		if (other == nullptr || other == m_actor) continue;

		Vec3 fwd = m_actor->m_orientation.GetForwardVector();
		Vec3 eyePos = m_actor->m_position + Vec3(0.f, 0.f, m_actor->m_eyeHeight);

		FOVZone priority = GetFOVZone(eyePos, fwd, other->m_position);
		prioritizeAgents.emplace_back(other, priority);
	}

	std::sort(prioritizeAgents.begin(), prioritizeAgents.end(),
		[](const auto& a, const auto& b)
		{
			return static_cast<int>(a.second) < static_cast<int>(b.second); // Lower zone enum = higher priority
		});

	for (const auto& [other, zone] : prioritizeAgents)
	{
		other->UpdateAIAgent();
		nearbyAI.emplace_back(&other->m_agent);
	}
}

void AIActor::HeighDeviationCheck()
{
	if (m_actor->m_position.z > ALLOWED_HEIGHT_DEVIATION)
	{
		if (m_aiPath.size() >= 2)
		{
			Vec3 currentWaypoint = m_aiPath[m_aiPath.size() - 1];
			Vec3 previousWaypoint = m_aiPath[m_aiPath.size() - 2];

			Vec3 agentPosition = m_actor->m_position;
			Vec3 pathSegment = previousWaypoint - currentWaypoint;
			Vec3 agentToWaypointStart = agentPosition - currentWaypoint;

			Vec3 projectionPoint = GetProjectedOnto3D(agentToWaypointStart, pathSegment);
			Vec3 projectedPositionOnPath = currentWaypoint + projectionPoint;
			//DebugAddWorldLine(m_actor->m_position, projectedPositionOnPath, 0.1f, 0.f, Rgba8::WISTERIA, Rgba8::WISTERIA, DebugRenderMode::ALWAYS);;
			m_actor->m_position.z = Interpolate(m_actor->m_position.z, projectedPositionOnPath.z, 0.2f);
		}
	}
}

void AIActor::RequestPathfindingJob(Vec3 startPoint, Vec3 goalPoint)
{
	if (m_isWaitingForPath || m_currentGame == nullptr || m_currentPath == nullptr) return;

	AStarPathfindingJob* job = new AStarPathfindingJob(this, startPoint, goalPoint);
	g_theJobSystem->QueueJob(job);
	m_isWaitingForPath = true;
}

void AIActor::AStar(Vec3 startPoint, Vec3 goalPoint, std::vector<Vec3>& outPath)
{
	double timeBefore = GetCurrentTimeSeconds();
	m_currentPath->ComputeAStar(startPoint, goalPoint, outPath);
	double timeAfter = GetCurrentTimeSeconds();

	float msElapsed = 1000.f * float(timeAfter - timeBefore);
	g_theConsole->AddLine(Rgba8::RED, Stringf("Generated a path of %i steps from triangle %i to triangle %i in %.02f ms", outPath.size(), startPoint, goalPoint, msElapsed));

	size_t pathSizeInBytes = outPath.size() * sizeof(size_t); // Approximate size of one path
	g_theConsole->AddLine(Rgba8::DARK_ORANGE, Stringf("The approximate size of one path is %zu bytes", pathSizeInBytes));

	m_currentGame->PrintAverageFPS();

	if (m_currentGame->m_enablePathVisual)
	{
		if (!outPath.empty())
		{
			for (size_t i = 0; i < outPath.size(); i++)
			{
				Vec3 point = outPath[i];
				Rgba8 pointColor = (i == 0) ? Rgba8::RED : (i == outPath.size() - 1 ? Rgba8::GREEN : Rgba8::MAGENTA);
				DebugAddWorldPoint(point, 0.1f, 8, 2.5f, pointColor, pointColor, DebugRenderMode::ALWAYS);

				if (i > 0)
				{
					Vec3 prevPoint = outPath[i - 1];
					DebugAddWorldLine(prevPoint, point, 0.1f, 2.5f, Rgba8::BUBBLEGUM_PINK, Rgba8::BUBBLEGUM_PINK, DebugRenderMode::ALWAYS);
				}
			}
		}
	}
}

void AStarPathfindingJob::Execute()
{
	m_ai->AStar(m_start, m_goal, m_resultPath);
	m_state = JobStatus::COMPLETED;
}
