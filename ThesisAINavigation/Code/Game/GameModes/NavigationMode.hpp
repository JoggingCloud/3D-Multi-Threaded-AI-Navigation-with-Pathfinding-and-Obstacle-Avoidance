#pragma once
#include "Game/Game.hpp"
#include "ThirdParty/TinyXML2/tinyxml2.h"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Core/Vertex_PCUTBN.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Renderer/Texture.hpp"
#include "Engine/Renderer/Terrain.hpp"
#include "Engine/Renderer/Material.hpp"
#include "Engine/Renderer/NavMesh.hpp"
#include "Engine/Utilities/Prop.hpp"
#include <vector>
#include <string>

class Controller;
class PlayerActor;
class Actor;
struct ActorUID;
class Shader;
class Texture;
class Image;
class VertexBuffer;
class IndexBuffer;
class NavMeshPathfinding;

class NavigationMode : public Game
{
public:
	NavigationMode() = default;
	NavigationMode(GameModeConfig const& config);
	~NavigationMode() = default;

	void Startup() override;
	void UpdateGameMode() override;
	void Render() override;
	void Shutdown() override;

private:
	Clock* m_clock = nullptr;

public:
	bool m_canSeeNavMesh = false;

	bool m_canSeeAiPath = false;
	bool m_canSeeAiOpenlist = false;
	bool m_hasAgentReachedGoal = false;

public:
	//Vec3 m_sunDirection = Vec3(0.f, 0.f, -1000.f);
	EulerAngles m_sunDirection = EulerAngles(120.f, 45.f, 0.f);
	float m_sunIntensity = 0.0f;
	float m_ambientIntensity = 1.f;

	float m_minFallOff = 0.1f;
	float m_maxFallOff = 1.f;
	float m_minFallOffMultiplier = 0.f;
	float m_maxFallOffMultiplier = 1.f;
};
