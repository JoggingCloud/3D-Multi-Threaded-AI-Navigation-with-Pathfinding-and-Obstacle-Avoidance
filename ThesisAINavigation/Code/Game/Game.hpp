#pragma once
#include "Engine/Core/EngineCommon.hpp"
#include "Game/GameCommon.hpp"
#include "Game/Map.hpp"
#include "Game/PlayGround.hpp"
#include "Game/PlayerActor.hpp"
#include "Game/ActorDefinitions.hpp"

#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "ThirdParty/ImGui/imgui.h"
#include <string>
#include <deque>

class Texture;
class PlayerActor;

struct GameModeConfig
{
	bool m_useAStar = false;
	bool m_useVO = false;
	bool m_useRVO = false;
	bool m_useHRVO = false;
	bool m_useORCA = false;
	bool m_useProps = false;

	IntVec2 m_terrianDimensions = IntVec2::ZERO;
	float m_groundHeight = 0.f;
	float m_hillHeight = 0.f;
	float m_mountainHeight = 0.f;

	int m_numberOfAgents = 0;
};

class Game
{
public:
	Game() = default;
	Game(GameModeConfig const& config); 
	virtual ~Game() = default;
	virtual void Startup() = 0;
	virtual void CreateSky();
	virtual void CreateSkyBuffers();
	virtual void FPSCalculation();
	virtual void PrintAverageFPS() const;

	virtual void Shutdown() = 0;
	void RunFrame();
	virtual void RenderSkyBox() const;
	void RenderGameTitle() const;
	void RenderFpsAndPlayerPos();
	virtual void RenderGameUI();
	virtual void RenderDebugKeyWindow();
	virtual void Render() = 0;
	virtual void UpdateGameMode() = 0;
	float GetGameFPS() const;

public:
	GameModeConfig m_gameModeConfig;
	Map* m_map = nullptr;
	PlayGround* m_playGround = nullptr;
	PlayerActor* m_player = nullptr;
	Clock* m_clock = nullptr;

	Camera* m_uiScreenView = nullptr;
	Camera* m_attractModeCamera = nullptr;
	Camera* m_lobbyModeCamera = nullptr;

	Texture* m_skyBoxTexture = nullptr;

	VertexBuffer* m_skyVertexBuffer = nullptr;
	IndexBuffer* m_skyIndexBuffer = nullptr;

	std::vector<unsigned int> m_skyIndexes;
	std::vector<Vertex_PCU> m_skyVertices;

public:
	std::deque<float> m_frameTimes; // Store last N frame times
	size_t m_maxFrameSamples = 60; // Limit to 60 samples

	float m_fpsUpdateTimer = 0.f;
	float m_smoothedFps = 60.f;
	float m_displayedFps = 60.f;

	std::deque<float> m_displayedFpsHistory;
	const int kMaxDisplayedFpsSamples = 100;
	float m_smoothedMaxFps = 180.f;

public:
	// --------------------Path finding-----------------------------
	bool m_enableNavMeshVisual = false;
	bool m_enableHeatmapVisual = false;
	bool m_enableBVHVisual = false;
	
	// Randomly selects an agent and shows their path 
	bool m_enablePathVisual = false;
	
	// ------------Obstacle Avoidance Mode only---------------------
	bool m_enableFOVZoneVisual = false;
	bool m_enableCurrentVOAlgorithmVisual = false;
};

inline ImVec2 operator+(const ImVec2& a, const ImVec2& b) { return ImVec2(a.x + b.x, a.y + b.y); }
