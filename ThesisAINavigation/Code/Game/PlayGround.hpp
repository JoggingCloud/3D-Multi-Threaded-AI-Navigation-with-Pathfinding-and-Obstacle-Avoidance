#pragma once
#include "ThirdParty/TinyXML2/tinyxml2.h"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Renderer/Texture.hpp"
#include "Engine/Renderer/Material.hpp"
#include "Engine/Utilities/Model.hpp"
#include "Engine/Utilities/Prop.hpp"
#include <vector>
#include <string>

class Controller;
class Game;
class PlayerActor;
class Actor;
struct ActorUID;

struct PlayGroundTempActorInfo
{
	Vec3 m_position = Vec3::ZERO;
	float m_radius = 0.f;
};

struct PlayGroundSpawnInfo
{
	std::string m_actorType = "";
	Vec3 m_actorPosition;
	EulerAngles m_actorOrientation;
};

class PlayGround
{
public:
	PlayGround() = default;
	PlayGround(Game* owner, const std::vector<PlayGroundSpawnInfo>& spawnInfos);
	virtual ~PlayGround();

	void LoadModel();
	void LoadGrid();
	void Render() const;
	void RenderPlayGround() const;
	void RenderActors() const;

	void PopulatePlayGroundWithActors(const std::vector<PlayGroundSpawnInfo>& spawnInfos);

	void PlayGroundUpdate();
	void UpdateActors();

	std::vector<Actor*> GetAllAgents() const;
	ActorUID GenerateActorUID(int actorIndex);
	Actor* SpawnActor(const PlayGroundSpawnInfo& spawnInfo);
	Actor* GetActorByUID(const ActorUID uid) const;

	void PlayGroundShutDown();

public:
	Game* m_game = nullptr;
	Model* m_playGroundModel = nullptr;
	Prop* m_gridProp = nullptr;

public:
	//Vec3 m_sunDirection = Vec3(0.f, 0.f, -1000.f);
	EulerAngles m_sunDirection = EulerAngles(120.f, 45.f, 0.f);
	float m_sunIntensity = 0.0f;
	float m_ambientIntensity = 1.f;

	float m_minFallOff = 0.1f;
	float m_maxFallOff = 1.f;
	float m_minFallOffMultiplier = 0.f;
	float m_maxFallOffMultiplier = 1.f;

private:
	std::vector<Actor*> m_agentActors;
	int m_maxNumAgents = 0;
	int m_agentID = 0;
	static const unsigned int MAX_ACTOR_SALT = 0x0000fffeu;
	unsigned int m_actorSalt = MAX_ACTOR_SALT;

	float m_maxDistance = 22.f;
};
