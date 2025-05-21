#pragma once
#include <vector>
#include "Game/Controller.hpp"
#include "Game/Game.hpp"
#include "Game/Map.hpp"
#include "Game/PlayGround.hpp"
#include "Engine/Core/JobSystem.hpp"
#include "Engine/Core/Timer.hpp"
#include "Engine/AI/ObstacleAvoidance.hpp"

constexpr float MAX_ANGLE_BEFORE_MOVEMENT = 5.f; // tweakable: 5–15 degrees is good range
constexpr float FOV_DEGREES = 180.f;
constexpr int NUM_CONE_SEGMENTS = 10;

enum class FOVZone
{
	CENTRAL,		// 0-40 degrees (Highest-priority)
	MID_PERIPHERY,	// 40-100 degrees
	FAR_PERPHERY,	// 100-130 degrees
	BEHIND			// 130-180 degrees (Lowest-priority)
};

struct LineTraceResult
{
	bool	m_didImpact             = false;
	float	m_impactDist            = 0.f;
	Vec3	m_impactPos             = Vec3::ZERO;
	Vec3	m_impactNormal          = Vec3::ZERO;
	Vec3    m_impactedActorVelocity = Vec3::ZERO;
	Actor*  m_impactedActor         = nullptr;
};

class AIActor : public Controller
{
public:
	AIActor() = default;
	AIActor(Game* game, Map* map, NavMesh* navMesh, NavMeshPathfinding* path);
	AIActor(Game* game, PlayGround* playGround);
	virtual ~AIActor() = default;

	virtual void Update() override;

	// ORCA helper functions for decision making 
	FOVZone GetFOVZone(Vec3 const& selfPosition, Vec3 const& fwdDir, Vec3 const& otherPosition);
	void AgentFOV(Vec3 const& originPos, Vec3 const& forwardDir, float angleRadius);

	void ApplyFinalMovement(Vec3 const& finalDirection, float maxTurnAngle, float maxAngleBeforeApplyingMovement);

	// Obstacle velocity 
	void CurrentObstacleMode();
	void GetNearbyAgentsOnTheMap(std::vector<Actor*>& agents, const Vec3& position, float radius);
	void GetNearbyAgentsOnThePlayGround(std::vector<Actor*>& agents, const Vec3& position, float radius);
	
	// Path construction and movement along path update
	void AiTraversalUpdate(Vec3 currentPos);
	void MoveAlongPathUpdate();
	void AgentPrioritization(std::vector<Actor*>& nearbyAgents, std::vector<AIAgent*>& nearbyAI);
	void HeighDeviationCheck();

	// A-Star
	void RequestPathfindingJob(Vec3 startPoint, Vec3 goalPoint);
	void AStar(Vec3 startPoint, Vec3 goalPoint, std::vector<Vec3>& outPath);

public:
	Game* m_currentGame = nullptr;
	Map* m_currentMap = nullptr;
	PlayGround* m_currentPlayGround = nullptr;
	NavMesh* m_currentNavMesh = nullptr;
	NavMeshPathfinding* m_currentPath = nullptr;
	ObstacleAvoidnace* m_obstacleAvoidance = nullptr;
	std::vector<Actor*> m_visibleActorsInLOS;

	Timer m_repathTimer;
	float m_repathPeriod = 0.f;
	float m_repathDuration = 0.f;
	float m_repathTimeRemaining = 0.f;
	float m_goalCheckTimer = 0.f;
	float m_searchRadius = 50.f;
	float m_progressAlongPath = 0.f;
	float m_maxLineTraceDistance = 5.f;
	float m_sightFOV = 180.f;

 	bool m_hasReachedGoal = true;
 	Vec3 m_goalPoint = Vec3::ZERO;

public:
	std::vector<Vec3> m_aiPath;
	LineTraceResult m_closestResult;
	ActorUID m_otherActorUID = ActorUID::INVALID;
	Actor* m_actor = nullptr;

private:
	bool m_isPathStored = false;
	Vec3 m_currentTargetPos;

public:
	bool m_isWaitingForPath = false; // Tracks if a path request is in progress
};

class AStarPathfindingJob : public Job 
{
public:
	AStarPathfindingJob(AIActor* ai, Vec3 start, Vec3 goal)
		: Job(JobType::AI), m_ai(ai), m_start(start), m_goal(goal) { m_state = JobStatus::NEW; }

	virtual void Execute() override;

	std::vector<Vec3> GetResult() const { return m_resultPath; }

public:
	AIActor* m_ai;
	Vec3 m_start;
	Vec3 m_goal;
	std::vector<Vec3> m_resultPath;
};
