#include "ObstacleAvoidanceMode.hpp"
#include "Game/PlayGround.hpp"

#include "Engine/Renderer/Window.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/DebugRenderer.hpp"

ObstacleAvoidanceMode::ObstacleAvoidanceMode(GameModeConfig const& config)
	: Game(config)
{
}

void ObstacleAvoidanceMode::GenerateSpawnInfosForCurrentMode(std::vector<PlayGroundSpawnInfo>& spawnInfos)
{
	if (m_gameModeConfig.m_numberOfAgents == 2 && m_gameModeConfig.m_useVO)
	{
		spawnInfos.emplace_back(PlayGroundSpawnInfo{ "Agent", Vec3(-10.f, 0.f, 0.f), EulerAngles(0.f, 0.f, 0.f) });
		spawnInfos.emplace_back(PlayGroundSpawnInfo{ "Agent", Vec3(10.f, 0.f, 0.f), EulerAngles(180.f, 0.f, 0.f) });
	}
	else if (m_gameModeConfig.m_numberOfAgents == 3 && m_gameModeConfig.m_useVO)
	{
		spawnInfos.emplace_back(PlayGroundSpawnInfo{ "Agent", Vec3(5.f, 1.f, 0.f), EulerAngles(180.f, 0.f, 0.f) });
		spawnInfos.emplace_back(PlayGroundSpawnInfo{ "Agent", Vec3(5.f, -1.f, 0.f), EulerAngles(180.f, 0.f, 0.f) });
		spawnInfos.emplace_back(PlayGroundSpawnInfo{ "Agent", Vec3(-5.f, 0.f, 0.f), EulerAngles(0.f, 0.f, 0.f) });
	}
	else if (m_gameModeConfig.m_numberOfAgents == 2 && m_gameModeConfig.m_useRVO)
	{
		spawnInfos.emplace_back(PlayGroundSpawnInfo{ "Agent", Vec3(-10.f, 0.f, 0.f), EulerAngles(0.f, 0.f, 0.f) });
		spawnInfos.emplace_back(PlayGroundSpawnInfo{ "Agent", Vec3(10.f, 0.f, 0.f), EulerAngles(180.f, 0.f, 0.f) });
	}
	else if (m_gameModeConfig.m_numberOfAgents == 3 && m_gameModeConfig.m_useRVO)
	{
		spawnInfos.emplace_back(PlayGroundSpawnInfo{ "Agent", Vec3(5.f, 1.f, 0.f), EulerAngles(180.f, 0.f, 0.f) });
		spawnInfos.emplace_back(PlayGroundSpawnInfo{ "Agent", Vec3(5.f, -1.f, 0.f), EulerAngles(180.f, 0.f, 0.f) });
		spawnInfos.emplace_back(PlayGroundSpawnInfo{ "Agent", Vec3(-5.f, 0.f, 0.f), EulerAngles(0.f, 0.f, 0.f) });
	}
	else if (m_gameModeConfig.m_numberOfAgents == 2 && m_gameModeConfig.m_useHRVO)
	{
		spawnInfos.emplace_back(PlayGroundSpawnInfo{ "Agent", Vec3(-10.f, 0.f, 0.f), EulerAngles(0.f, 0.f, 0.f) });
		spawnInfos.emplace_back(PlayGroundSpawnInfo{ "Agent", Vec3(10.f, 0.f, 0.f), EulerAngles(180.f, 0.f, 0.f) });
	}
	else if (m_gameModeConfig.m_numberOfAgents == 3 && m_gameModeConfig.m_useHRVO)
	{
		spawnInfos.emplace_back(PlayGroundSpawnInfo{ "Agent", Vec3(5.f, 1.f, 0.f), EulerAngles(180.f, 0.f, 0.f) });
		spawnInfos.emplace_back(PlayGroundSpawnInfo{ "Agent", Vec3(5.f, -1.f, 0.f), EulerAngles(180.f, 0.f, 0.f) });
		spawnInfos.emplace_back(PlayGroundSpawnInfo{ "Agent", Vec3(-5.f, 0.f, 0.f), EulerAngles(0.f, 0.f, 0.f) });
	}
	else if (m_gameModeConfig.m_numberOfAgents == 2 && m_gameModeConfig.m_useORCA)
	{
		spawnInfos.emplace_back(PlayGroundSpawnInfo{ "Agent", Vec3(-2.5f, 0.f, 0.f), EulerAngles(0.f, 0.f, 0.f) });
		spawnInfos.emplace_back(PlayGroundSpawnInfo{ "Agent", Vec3(2.5f, 0.f, 0.f), EulerAngles(180.f, 0.f, 0.f) });
	}
	else if (m_gameModeConfig.m_numberOfAgents == 3 && m_gameModeConfig.m_useORCA)
	{
		spawnInfos.emplace_back(PlayGroundSpawnInfo{ "Agent", Vec3(2.5f, 0.6f, 0.f), EulerAngles(180.f, 0.f, 0.f) });
		spawnInfos.emplace_back(PlayGroundSpawnInfo{ "Agent", Vec3(2.5f, -0.6f, 0.f), EulerAngles(180.f, 0.f, 0.f) });
		spawnInfos.emplace_back(PlayGroundSpawnInfo{ "Agent", Vec3(-2.5f, 0.f, 0.f), EulerAngles(0.f, 0.f, 0.f) });
	}
	else if (m_gameModeConfig.m_numberOfAgents == 10 && m_gameModeConfig.m_useORCA)
	{
		float outerRadius = 15.f;
		float innerRadius = 8.f; // Smaller radius for inner circle
		int numOuterAgents = 5;
		int numInnerAgents = 5;

		for (int i = 0; i < numOuterAgents; i++)
		{
			float angleDegrees = i * (360.f / numOuterAgents);
			float angleRadians = ConvertDegreesToRadians(angleDegrees);
			Vec3 position = Vec3(outerRadius * cosf(angleRadians), outerRadius * sinf(angleRadians), 0.f);
			EulerAngles orientation = EulerAngles(angleDegrees + 180.f, 0.f, 0.f);
			spawnInfos.emplace_back(PlayGroundSpawnInfo{ "Agent", position, orientation });
		}

		for (int i = 0; i < numInnerAgents; i++)
		{
			// Use offset angles between outer agents
			float outerAngleStep = 360.f / numOuterAgents;
			float angleDegrees = (i * outerAngleStep) + (outerAngleStep * 0.5f); // Midpoint between two outer agents
			float angleRadians = ConvertDegreesToRadians(angleDegrees);
			Vec3 position = Vec3(innerRadius * cosf(angleRadians), innerRadius * sinf(angleRadians), 0.f);
			EulerAngles orientation = EulerAngles(angleDegrees + 180.f, 0.f, 0.f);
			spawnInfos.emplace_back(PlayGroundSpawnInfo{ "Agent", position, orientation });
		}
	}
	else if (m_gameModeConfig.m_numberOfAgents == 20 && m_gameModeConfig.m_useORCA)
	{
		float outerRadius = 15.f;
		int numAgents = m_gameModeConfig.m_numberOfAgents;

		for (int i = 0; i < numAgents; i++)
		{
			float angleDegrees = i * (360.f / numAgents);
			float angleRadians = ConvertDegreesToRadians(angleDegrees);
			Vec3 position = Vec3(outerRadius * cosf(angleRadians), outerRadius * sinf(angleRadians), 0.f);
			EulerAngles orientation = EulerAngles(angleDegrees + 180.f, 0.f, 0.f);
			spawnInfos.emplace_back(PlayGroundSpawnInfo{ "Agent", position, orientation });
		}
	}
}

void ObstacleAvoidanceMode::Startup()
{
	m_uiScreenView = new Camera();
	m_uiScreenView->SetOrthoView(Vec2(0.f, 0.f), Vec2((float)g_theWindow->GetClientDimensions().x, (float)g_theWindow->GetClientDimensions().y));

	g_bitmapFont = g_theRenderer->CreateOrGetBitmapFont("Data/Fonts/SquirrelFixedFont.png");

	g_theRenderer->SetDepthMode(DepthMode::ENABLED);
	g_theRenderer->SetRasterizerState(RasterizerMode::SOLID_CULL_NONE);
	g_theRenderer->SetBlendMode(BlendMode::OPAQUE);

	CreateSky();
	CreateSkyBuffers();

	ActorDefinition::InitializeActorDef();

	m_player = new PlayerActor(this, Vec3(0.f, 0.f, 10.f), EulerAngles(0.f, 90.f, 0.f));

	ActorDefinition* agentDef = ActorDefinition::GetActorDefByName("Agent");
	if (!agentDef)
	{
		ERROR_AND_DIE("Failed to get the agent actor definition");
	}

	std::vector<PlayGroundSpawnInfo> spawnInfos;
	GenerateSpawnInfosForCurrentMode(spawnInfos);
	m_playGround = new PlayGround(this, spawnInfos);
}

void ObstacleAvoidanceMode::UpdateGameMode()
{
	FPSCalculation();
	m_player->Update();
	m_playGround->PlayGroundUpdate();
}

void ObstacleAvoidanceMode::Render()
{
	g_theRenderer->BeginCamera(m_player->m_playerWorldView);

	RenderSkyBox();
	m_playGround->Render();

	DebugRenderWorld(m_player->m_playerWorldView);

	g_theRenderer->EndCamera(m_player->m_playerWorldView);

	g_theRenderer->BeginCamera(*m_uiScreenView);

	RenderGameUI();
	RenderDebugKeyWindow();

	g_theRenderer->EndCamera(*m_uiScreenView);
}

void ObstacleAvoidanceMode::Shutdown()
{
	SafeDelete(m_playGround);
	m_skyIndexes.clear();
	m_skyVertices.clear();
	SafeDelete(m_skyIndexBuffer);
	SafeDelete(m_skyVertexBuffer);
}

