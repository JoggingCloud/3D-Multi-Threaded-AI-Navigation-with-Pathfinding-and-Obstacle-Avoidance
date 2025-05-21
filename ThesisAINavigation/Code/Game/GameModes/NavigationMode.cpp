#include "NavigationMode.hpp"
#include "Game/Map.hpp"

#include "Engine/Renderer/Window.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/DebugRenderer.hpp"
#include "Engine/Renderer/Terrain.hpp"
#include "Engine/AI/Pathfinding/NavMeshPathfinding.hpp"

NavigationMode::NavigationMode(GameModeConfig const& config)
	: Game(config)
{
}

void NavigationMode::Startup()
{
	m_uiScreenView = new Camera();
	m_uiScreenView->SetOrthoView(Vec2(0.f, 0.f), Vec2((float)g_theWindow->GetClientDimensions().x, (float)g_theWindow->GetClientDimensions().y));

	g_bitmapFont = g_theRenderer->CreateOrGetBitmapFont("Data/Fonts/SquirrelFixedFont.png");

	g_theRenderer->SetDepthMode(DepthMode::ENABLED);
	g_theRenderer->SetRasterizerState(RasterizerMode::SOLID_CULL_NONE);
	g_theRenderer->SetBlendMode(BlendMode::OPAQUE);

	ActorDefinition::InitializeActorDef();

	CreateSky();
	CreateSkyBuffers();

	m_player = new PlayerActor(this, Vec3(0.f, 0.f, 10.f), EulerAngles(0.f, 90.f, 0.f));

	MapConfig astarModeConfig;
	astarModeConfig.m_groundHeightThreshold = m_gameModeConfig.m_groundHeight;
	astarModeConfig.m_hillHeightTheshold = m_gameModeConfig.m_hillHeight;
	astarModeConfig.m_mountainHeightThreshold = m_gameModeConfig.m_mountainHeight;
	astarModeConfig.m_terrainWidthDimension = m_gameModeConfig.m_terrianDimensions.x;
	astarModeConfig.m_terrainHeightDimension = m_gameModeConfig.m_terrianDimensions.y;
	m_map = new Map(this, astarModeConfig, m_gameModeConfig.m_numberOfAgents);
}

void NavigationMode::UpdateGameMode()
{
	FPSCalculation();
	m_player->Update();
	m_map->MapUpdate();
}

void NavigationMode::Render()
{
	g_theRenderer->BeginCamera(m_player->m_playerWorldView);

	RenderSkyBox();
	m_map->Render();

	DebugRenderWorld(m_player->m_playerWorldView);

	g_theRenderer->EndCamera(m_player->m_playerWorldView);

	g_theRenderer->BeginCamera(*m_uiScreenView);

	RenderGameUI();
	RenderDebugKeyWindow();

	g_theRenderer->EndCamera(*m_uiScreenView);
}

void NavigationMode::Shutdown()
{
	SafeDelete(m_map);
	m_skyIndexes.clear();
	m_skyVertices.clear();
	SafeDelete(m_skyIndexBuffer);
	SafeDelete(m_skyVertexBuffer);
}
