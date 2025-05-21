#include "Game/PlayGround.hpp"
#include "Engine/Renderer/DebugRenderer.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/Shader.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/XmlUtils.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Core/Image.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Game/Controller.hpp"
#include "Game/Actor.hpp"
#include "Game/ActorDefinitions.hpp"
#include "Game/Game.hpp"
#include "Game/PlayerActor.hpp"
#include "Game/GameCommon.hpp"
#include <algorithm>


PlayGround::PlayGround(Game* owner, const std::vector<PlayGroundSpawnInfo>& spawnInfos)
	: m_game(owner), m_maxNumAgents(static_cast<int>(spawnInfos.size()))
{
	LoadModel();
	LoadGrid();
	PopulatePlayGroundWithActors(spawnInfos);
}

PlayGround::~PlayGround()
{
	PlayGroundShutDown();
}

void PlayGround::LoadModel()
{
	m_playGroundModel = new Model();
	std::string objfilePath = "Data/Models/PlayGround.obj";
	if (!m_playGroundModel->Load(objfilePath))
	{
		ERROR_AND_DIE("Error: Could not load OBJ File: %s\n");
	}
}

void PlayGround::LoadGrid()
{
	m_gridProp = new Prop(Vec3::ZERO, EulerAngles::ZERO);
	m_gridProp->CreateGrid(50, 5, 0.05f);
	m_gridProp->CreateVertexPCUBuffer();
}

void PlayGround::Render() const
{
	RenderPlayGround();
	RenderActors();
}

void PlayGround::RenderPlayGround() const
{
	g_theRenderer->SetLightConstants(m_sunDirection.GetForwardVector(), m_sunIntensity, m_ambientIntensity, m_game->m_player->m_position, m_minFallOff, m_maxFallOff, m_minFallOffMultiplier, m_maxFallOffMultiplier, LightingDebug());

	m_playGroundModel->RenderLoadedModel();
	m_gridProp->RenderPropVBO();
}

void PlayGround::RenderActors() const
{
	if (!m_agentActors.empty())
	{
		for (int index = 0; index < m_agentActors.size(); index++)
		{
			if (m_agentActors[index] == nullptr)
			{
				continue;
			}
			m_agentActors[index]->Render();
		}
	}
}

void PlayGround::PopulatePlayGroundWithActors(const std::vector<PlayGroundSpawnInfo>& spawnInfos)
{
	for (PlayGroundSpawnInfo const& info : spawnInfos)
	{
		SpawnActor(info);
	}
}

void PlayGround::PlayGroundUpdate()
{
	UpdateActors();
}

void PlayGround::UpdateActors()
{
	if (!m_agentActors.empty())
	{
		for (int index = 0; index < m_agentActors.size(); index++)
		{
			if (m_agentActors[index] == nullptr)
			{
				continue;
			}
			m_agentActors[index]->Update();
		}
	}
}

std::vector<Actor*> PlayGround::GetAllAgents() const
{
	return m_agentActors;
}

ActorUID PlayGround::GenerateActorUID(int actorIndex)
{
	ActorUID uid = ActorUID(m_actorSalt++, actorIndex);
	return uid;
}

Actor* PlayGround::SpawnActor(const PlayGroundSpawnInfo& spawnInfo)
{
	static int actorCounter = 0;

	for (int i = 0; i < m_agentActors.size(); i++)
	{
		if (m_agentActors[i] == nullptr)
		{
			ActorUID uid = GenerateActorUID(i);

			Actor* newActor = new Actor(this, m_game, spawnInfo, uid);

			newActor->m_uniqueName = Stringf("%s%i", newActor->m_actorDef->m_name.c_str(), actorCounter++);

			m_agentActors[i] = newActor;
			return newActor;
		}
	}

	int index = static_cast<int>(m_agentActors.size());
	ActorUID uid = GenerateActorUID(index);

	Actor* newActor = new Actor(this, m_game, spawnInfo, uid);

	newActor->m_uniqueName = Stringf("%s%i", newActor->m_actorDef->m_name.c_str(), actorCounter++);

	m_agentActors.emplace_back(newActor);

	return newActor;
}

Actor* PlayGround::GetActorByUID(const ActorUID uid) const
{
	for (Actor* actor : m_agentActors)
	{
		if (actor && actor->GetUID() == uid)
		{
			return actor;
		}
	}
	return nullptr;
}

void PlayGround::PlayGroundShutDown()
{
	SafeDelete(m_agentActors);
	SafeDelete(m_playGroundModel);
	SafeDelete(m_gridProp);
}
