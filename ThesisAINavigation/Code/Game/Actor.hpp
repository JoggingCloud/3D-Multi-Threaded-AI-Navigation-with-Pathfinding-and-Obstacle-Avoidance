#pragma once
#include "Game/ActorUID.hpp"
#include "Engine/AI/ObstacleAvoidance.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Renderer/VertexBuffer.hpp"
#include "Engine/Renderer/IndexBuffer.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Renderer/SpriteAnimDefinition.hpp"
#include <vector>

class Clock;
class Map;
class PlayGround;
class Game;
class Controller;
class AIActor;
class Timer;
class Shader;
class Camera;
class VertexBuffer;
class IndexBuffer;
struct MapSpawnInfo;
struct PlayGroundSpawnInfo;
struct ActorDefinition;

class Actor
{
public:
	Actor(Map* owner, Game* game, MapSpawnInfo mapSpawnInfo, ActorUID actorUID);
	Actor(PlayGround* owner, Game* game, PlayGroundSpawnInfo playGroundSpawnInfo, ActorUID actorUID);
	Actor() = default;
	~Actor();

public:
	void CreateZAlignedAgent();
	void CreateBuffers();
	void Update();
	void Render();
	Mat44 GetModelMatrix() const;

	ActorUID GetUID() const;
	void SetUID(ActorUID uid);
	Controller* GetController() const;
	AIActor* GetAiController() const;

	// Movement 
	void UpdatePhysiscs();
	void AddForce(const Vec3& force);
	void AddImpulse(const Vec3& impulse);
	void MoveInDirection(Vec3 direction, float speed);
	void TurnInDirection(float goalDegree, float maxAngle);

	AIAgent m_agent;

	void UpdateAIAgent()
	{
		m_agent.m_position = m_position;
		m_agent.m_velocity = m_velocity;
		m_agent.m_preferredVelocity = m_preferredVelocity;
		m_agent.m_orientation = m_orientation;
		m_agent.m_searchRadius = m_searchRadius;
		m_agent.m_physicsRadius = m_physicsRadius;
		m_agent.m_moveSpeed = m_moveSpeed;
	}

	void OnPossessed(Controller* controller);
	void OnUnPossessed(Controller* controller);

	Vec3 GetActorPosition() const;

	std::string GetName() const { return m_uniqueName; }

public:
	Map* m_map = nullptr;
	Game* m_currentGame = nullptr;
	PlayGround* m_playGround = nullptr;
	ActorDefinition* m_actorDef = nullptr;
	
	std::string m_actorDefName;
	
	ActorUID m_uid;
	ActorUID m_ownerUID;

	Vec3 m_initialDirection = Vec3::ZERO;
	Vec3 m_acceleration = Vec3::ZERO;
	Vec3 m_velocity = Vec3::ZERO;
	Vec3 m_preferredVelocity = Vec3::ZERO;
	Vec3 m_position = Vec3::ZERO;
	Vec3 m_previousPosition = Vec3::ZERO;
	
	EulerAngles m_orientation = EulerAngles::ZERO;
	
	Rgba8 m_bodyColor = Rgba8::WHITE;
	Rgba8 m_eyeColor = Rgba8::WHITE;
	Rgba8 m_originalColor = Rgba8::YELLOW;

	std::string m_actorName = "";
	std::string m_actorFaction = "";
	std::string m_uniqueName = "";

	float m_moveSpeed = 0.f;
	float m_turnSpeed = 0.f;
	float m_physicsRadius = 0.f;
	float m_physicsHeight = 0.f;
	float m_eyeHeight = 0.f;
	float m_sightRadii = 0.f;
	float m_sightAngle = 0.f;
	float m_dragForce = 0.f;

	float m_minTimeHorizon = 0.1f;
	float m_maxTimeHorizon = 3.f; // Mess with value. Higher value will allow agents to make smoother and gradual adjustments. Lower value means more frequent changes and react only when they're very close to collisions.
	float m_combinedRadius = 0.f;
	float m_avoidanceFactor = 50.f;
	float m_searchRadius = 0;
	
	bool m_canBePossessed = false;
	bool m_isVisible = false;
	bool m_isAI = false;

public:
	VertexBuffer* m_bodyVertexBuffer = nullptr;
	IndexBuffer* m_bodyIndexBuffer = nullptr;

	std::vector<unsigned int> m_actorBodyIndexes;
	std::vector<Vertex_PCU> m_actorBodyVertices;

	VertexBuffer* m_eyeVertexBuffer = nullptr;
	IndexBuffer* m_eyeIndexBuffer = nullptr;

	std::vector<unsigned int> m_actorEyeIndexes;
	std::vector<Vertex_PCU> m_actorEyeVertices;

public:
	Controller* m_owningController = nullptr;
	AIActor* m_aiController = nullptr;
};
