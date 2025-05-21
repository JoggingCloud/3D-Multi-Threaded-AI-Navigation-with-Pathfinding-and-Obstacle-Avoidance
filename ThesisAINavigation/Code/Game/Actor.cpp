#include "Game/Actor.hpp"
#include "Game/App.hpp"
#include "Game/PlayerActor.hpp"
#include "Game/ActorDefinitions.hpp"
#include "Game/Game.hpp"
#include "Game/Controller.hpp"
#include "Game/AIActor.hpp"
#include "Game/Game.hpp"
#include "Game/Map.hpp"
#include "Game/PlayGround.hpp"

#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/DebugRenderer.hpp"
#include "Engine/Renderer/VertexBuffer.hpp"
#include "Engine/Renderer/IndexBuffer.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Renderer/Shader.hpp"
#include "Engine/Renderer/SpriteSheet.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Core/EngineCommon.hpp"

Actor::Actor(Map* owner, Game* game, MapSpawnInfo mapSpawnInfo, ActorUID actorUID)
{
	m_map = owner;
	m_currentGame = game;
	m_actorDefName = mapSpawnInfo.m_actorType;
	m_position = mapSpawnInfo.m_actorPosition;
	m_previousPosition = m_position;
	m_orientation = mapSpawnInfo.m_actorOrientation;
	m_initialDirection = m_orientation.GetForwardVector();
	m_actorDef = ActorDefinition::GetActorDefByName(m_actorDefName);

	if (m_actorDef)
	{
		m_actorName = m_actorDef->m_name;
		m_isAI = m_actorDef->m_aiEnabled;
		m_actorFaction = m_actorDef->m_faction;

		m_moveSpeed = m_actorDef->m_moveSpeed.GetRandomValueInRange();
		m_turnSpeed = m_actorDef->m_turnSpeed;

		m_physicsHeight = m_actorDef->m_height;
		m_eyeHeight = m_actorDef->m_eyeHeight;
		m_physicsRadius = m_actorDef->m_radius.GetRandomValueInRange();

		m_canBePossessed = m_actorDef->m_canBePossed;

		m_isVisible = m_actorDef->m_visible;
		m_dragForce = m_actorDef->m_drag;

		if (m_actorDef->m_name.find("Agent") != std::string::npos) // Check if the name contains "Agent"
		{
			CreateZAlignedAgent();
			CreateBuffers();
			m_bodyColor = Rgba8::SLATE_GRAY;
			m_eyeColor = Rgba8::RED;
		}
	}

	m_uid = actorUID;

	if (m_isAI)
	{
		m_aiController = new AIActor(m_currentGame, m_map, m_map->m_navMesh, m_map->m_aiPath);
		m_aiController->Possess(this);
	}
}

Actor::Actor(PlayGround* owner, Game* game, PlayGroundSpawnInfo playGroundSpawnInfo, ActorUID actorUID)
{
	m_playGround = owner;
	m_currentGame = game;
	m_actorDefName = playGroundSpawnInfo.m_actorType;
	m_position = playGroundSpawnInfo.m_actorPosition;
	m_previousPosition = m_position;
	m_orientation = playGroundSpawnInfo.m_actorOrientation;
	m_initialDirection = m_orientation.GetForwardVector();
	m_actorDef = ActorDefinition::GetActorDefByName(m_actorDefName);

	if (m_actorDef)
	{
		m_actorName = m_actorDef->m_name;
		m_isAI = m_actorDef->m_aiEnabled;
		m_actorFaction = m_actorDef->m_faction;

		m_moveSpeed = m_actorDef->m_moveSpeed.GetRandomValueInRange();
		m_turnSpeed = m_actorDef->m_turnSpeed;

		m_physicsHeight = m_actorDef->m_height;
		m_eyeHeight = m_actorDef->m_eyeHeight;
		m_physicsRadius = m_actorDef->m_radius.GetRandomValueInRange();

		m_canBePossessed = m_actorDef->m_canBePossed;

		m_isVisible = m_actorDef->m_visible;
		m_dragForce = m_actorDef->m_drag;

		if (m_actorDef->m_name.find("Agent") != std::string::npos) // Check if the name contains "Agent"
		{
			CreateZAlignedAgent();
			CreateBuffers();
			m_bodyColor = Rgba8::SLATE_GRAY;
			m_eyeColor = Rgba8::RED;
		}
	}

	m_uid = actorUID;

	if (m_isAI)
	{
		m_aiController = new AIActor(m_currentGame, m_playGround);
		m_aiController->Possess(this);
	}
}

Actor::~Actor()
{
	SafeDelete(m_bodyVertexBuffer);
	SafeDelete(m_bodyIndexBuffer);

	m_actorBodyIndexes.clear();
	m_actorBodyVertices.clear();

	SafeDelete(m_eyeVertexBuffer);
	SafeDelete(m_eyeIndexBuffer);

	m_actorEyeIndexes.clear();
	m_actorEyeVertices.clear();
}

void Actor::CreateZAlignedAgent()
{
	AddVertsForZCapsule3D(m_actorBodyVertices, m_actorBodyIndexes, Capsule3(Vec3(0.f, 0.f, m_physicsRadius), Vec3(0.f, 0.f, m_physicsHeight), m_physicsRadius), Rgba8::WHITE, AABB2::ZERO_TO_ONE, 16, 16);
	AddVertsForCone3D(m_actorEyeVertices, m_actorEyeIndexes, Vec3(m_physicsRadius * 0.7f, 0.f, m_eyeHeight), Vec3(m_physicsRadius * 0.9f, 0.f, m_eyeHeight), m_physicsRadius, m_physicsRadius * 0.5f, 32, Rgba8::WHITE);
}

void Actor::CreateBuffers()
{
	m_bodyVertexBuffer = g_theRenderer->CreateVertexBuffer(m_actorBodyVertices.size());
	g_theRenderer->CopyCPUToGPU(m_actorBodyVertices.data(), m_actorBodyVertices.size() * sizeof(Vertex_PCU), m_bodyVertexBuffer);

	m_bodyIndexBuffer = g_theRenderer->CreateIndexBuffer(m_actorBodyIndexes.size());
	g_theRenderer->CopyCPUToGPU(m_actorBodyIndexes.data(), m_actorBodyIndexes.size() * sizeof(unsigned int), m_bodyIndexBuffer);

	m_eyeVertexBuffer = g_theRenderer->CreateVertexBuffer(m_actorEyeVertices.size());
	g_theRenderer->CopyCPUToGPU(m_actorEyeVertices.data(), m_actorEyeVertices.size() * sizeof(Vertex_PCU), m_eyeVertexBuffer);

	m_eyeIndexBuffer = g_theRenderer->CreateIndexBuffer(m_actorEyeIndexes.size());
	g_theRenderer->CopyCPUToGPU(m_actorEyeIndexes.data(), m_actorEyeIndexes.size() * sizeof(unsigned int), m_eyeIndexBuffer);
}

void Actor::Update()
{
	if (m_currentGame->m_gameModeConfig.m_useORCA)
	{
		// #TODO make sure these values match up with what's in the xml so everything works properly
		float threshold = 0.5f; // Midpoint of the moveSpeed range defined in the XML
		float scaleFactor = (m_moveSpeed > threshold) ? 3.f : 4.f; // Adjust based on speed;
		m_searchRadius = m_physicsRadius * scaleFactor; // Initial search radius
	}
	else
	{
		m_searchRadius = 10.f;
	}

	m_owningController->Update();

	UpdatePhysiscs();
}

void Actor::Render()
{
	std::vector<Vertex_PCU> actorVerts;

	g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
	g_theRenderer->SetDepthMode(DepthMode::ENABLED);
	g_theRenderer->SetRasterizerState(RasterizerMode::SOLID_CULL_NONE);
	g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
	g_theRenderer->SetModelConstants(GetModelMatrix(), m_bodyColor);
	g_theRenderer->BindTexture(0, nullptr);
	g_theRenderer->BindShader(nullptr);
	g_theRenderer->DrawVertexBufferIndex(m_bodyVertexBuffer, m_bodyIndexBuffer, VertexType::Vertex_PCU, static_cast<int>(m_actorBodyIndexes.size()));

	g_theRenderer->SetModelConstants(GetModelMatrix(), m_eyeColor);
	g_theRenderer->DrawVertexBufferIndex(m_eyeVertexBuffer, m_eyeIndexBuffer, VertexType::Vertex_PCU, static_cast<int>(m_actorEyeIndexes.size()));
}

Mat44 Actor::GetModelMatrix() const
{
	Mat44 translation = Mat44::CreateTranslation3D(m_position);
	Mat44 orientation = m_orientation.GetAsMatrix_IFwd_JLeft_KUp();

	translation.Append(orientation);
	return translation;
}

ActorUID Actor::GetUID() const
{
	return m_uid;
}

void Actor::SetUID(ActorUID uid)
{
	m_uid = uid;
}

Controller* Actor::GetController() const
{
	return m_owningController;
}

AIActor* Actor::GetAiController() const
{
	return m_aiController;
}

void Actor::UpdatePhysiscs()
{
	m_previousPosition = m_position;

	float deltaSeconds = m_currentGame->m_clock->GetDeltaSeconds();

	Vec3 drag = -m_velocity;
	AddForce(drag);

	// Update velocity 
	m_velocity += m_acceleration * deltaSeconds * 0.5f;

	// update position
	m_position += m_velocity * deltaSeconds;

	m_velocity += m_acceleration * deltaSeconds * 0.5f;

	// Reset acceleration for next frame
	m_acceleration = Vec3::ZERO;
}

void Actor::AddForce(const Vec3& force)
{
	m_acceleration += force * m_dragForce;
}

void Actor::AddImpulse(const Vec3& impulse)
{
	m_velocity += impulse;
}

void Actor::MoveInDirection(Vec3 direction, float speed)
{
	Vec3 normalizedDirection = direction.GetNormalized();
	AddForce(normalizedDirection * speed);
}

void Actor::TurnInDirection(float goalDegree, float maxAngle)
{
	m_orientation.m_yawDegrees = GetTurnedTowardDegrees(m_orientation.m_yawDegrees, goalDegree, maxAngle);
}

void Actor::OnPossessed(Controller* controller)
{
	m_owningController = controller;
}

void Actor::OnUnPossessed(Controller* controller)
{
	// first set current controller to nullptr 
	controller = nullptr;
	if (m_isAI)
	{
		controller = m_aiController;
	}
}

Vec3 Actor::GetActorPosition() const
{
	return m_position;
}
