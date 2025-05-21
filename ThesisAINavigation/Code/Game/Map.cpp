#include "Map.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/DebugRenderer.hpp"
#include "Engine/Renderer/Shader.hpp"
#include "Engine/Renderer/NavMesh.hpp"
#include "Engine/AI/Pathfinding/NavMeshPathfinding.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/XmlUtils.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Core/Image.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/RaycastUtils.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Game/Controller.hpp"
#include "Game/Actor.hpp"
#include "Game/ActorDefinitions.hpp"
#include "Game/Game.hpp"
#include "Game/PlayerActor.hpp"
#include "Game/GameCommon.hpp"
#include <algorithm>

extern Renderer* g_theRenderer;

Map::Map(Game* owner, MapConfig const& config, int numAgentsToSpawn)
	: m_game(owner), m_mapConfig(config), m_maxNumAgents(numAgentsToSpawn)
{
	std::vector<std::string> regionNames = { "Ground", "Hill", "Mountain" };
	std::vector<Rgba8> regionColors = { Rgba8::BLUE, Rgba8::GREEN, Rgba8::RED };
	std::vector<float> regionHeights = { m_mapConfig.m_groundHeightThreshold, m_mapConfig.m_hillHeightTheshold, m_mapConfig.m_mountainHeightThreshold };
	m_terrain = new Terrain(m_mapConfig.m_terrainWidthDimension, m_mapConfig.m_terrainHeightDimension, NUM_REGIONS, TILING_FACTOR, regionNames, regionColors, regionHeights);
	
	std::string shaderName = "Data/Shaders/Terrain";
	m_terrainShader = g_theRenderer->CreateOrGetShader(shaderName.c_str(), VertexType::Vertex_PCUTBN);
	
	LoadTerrainMaterial();
	CreateTerrainBuffers();

	m_navMesh = new NavMesh();
	GenerateNavMesh();

	m_aiPath = new NavMeshPathfinding(m_navMesh);
	PopulateMapWithAgentActors();
}

Map::~Map()
{
	MapShutDown();
}

void Map::LoadTerrainMaterial()
{
	std::string grassDirtMaterialPath = "Data/Materials/GrassDirt.xml";
	m_grassDirtMat = new Material();
	if (!m_grassDirtMat->Load(grassDirtMaterialPath))
	{
		GUARANTEE_OR_DIE(false, "Invalid GrassDirt material in XML");
	}

	std::string rockMaterialPath = "Data/Materials/Rock.xml";
	m_rockMat = new Material();
	if (!m_rockMat->Load(rockMaterialPath))
	{
		GUARANTEE_OR_DIE(false, "Invalid Rock material in XML");
	}

	std::string snowRockMaterialPath = "Data/Materials/SnowRock.xml";
	m_snowRockMat = new Material();
	if (!m_snowRockMat->Load(snowRockMaterialPath))
	{
		GUARANTEE_OR_DIE(false, "Invalid Snow Rock material in XML");
	}
}

void Map::CreateTerrainBuffers()
{
	m_terrainVertexBuffer = g_theRenderer->CreateVertexBuffer(m_terrain->m_vertices.size());
	g_theRenderer->CopyCPUToGPU(m_terrain->m_vertices.data(), m_terrain->m_vertices.size() * sizeof(Vertex_PCUTBN), m_terrainVertexBuffer);

	m_terrainIndexBuffer = g_theRenderer->CreateIndexBuffer(m_terrain->m_indices.size());
	g_theRenderer->CopyCPUToGPU(m_terrain->m_indices.data(), m_terrain->m_indices.size() * sizeof(unsigned int), m_terrainIndexBuffer);
}

void Map::GenerateNavMesh()
{
	std::vector<Vec3> vertices;
	std::vector<int> vertexMapping;
	
	int mapWidth = static_cast<int>(m_terrain->m_terrainWidth);
	int mapHeight = static_cast<int>(m_terrain->m_terrainHeight);

	float xOffset = static_cast<float>(mapWidth) / 2.f;
	float yOffset = static_cast<float>(mapHeight) / 2.f;
	float rowOffset = 0.25f; // Offset for alternating rows
	float maxDisplacement = 0.3f; // Maximum displacement

	float maxAgentRadius = 2.f;
	int borderSkip = static_cast<int>(ceil(maxAgentRadius));

	// Generate vertices based on the height map
	for (int y = 0; y < mapHeight; y++)
	{
		for (int x = 0; x < mapWidth; x++)
		{
			float height = m_terrain->m_terrainHeightMap[Get1DIndex(x, y, mapWidth)];

			bool isCushionArea = (x < borderSkip || x >= mapWidth - borderSkip ||
				y < borderSkip || y >= mapHeight - borderSkip);

			bool isBorderArea = (x == borderSkip || x == mapWidth - 1 - borderSkip ||
				y == borderSkip || y == mapHeight - 1 - borderSkip);

			if (isCushionArea)
			{
				vertexMapping.emplace_back(-1);
			}
			else if (isBorderArea) 
			{ 
				float adjustedX = static_cast<float>(x) - xOffset;
				float adjustedY = static_cast<float>(y) - yOffset;
				vertices.emplace_back(Vec3(adjustedX, adjustedY, height * TERRAIN_HEIGHT - TERRAIN_HEIGHT * 0.5f + NAVMESH_ZBIAS));
				vertexMapping.emplace_back(static_cast<int>(vertices.size()) - 1);
				continue; 
			}
			else
			{
				// Altering row offset for odd rows
				float adjustedX = static_cast<float>(x) - xOffset + ((y % 2 != 0) ? rowOffset : 0.f);
				float adjustedY = static_cast<float>(y) - yOffset;

				// Random displacement
				float randomXOffset = g_rng.SRollRandomFloatInRange(-maxDisplacement, maxDisplacement);
				float randomYOffset = g_rng.SRollRandomFloatInRange(-maxDisplacement, maxDisplacement);

				adjustedX += randomXOffset;
				adjustedY += randomYOffset;

				vertices.emplace_back(Vec3(adjustedX, adjustedY, height * TERRAIN_HEIGHT - TERRAIN_HEIGHT * 0.5f + NAVMESH_ZBIAS));
				vertexMapping.emplace_back(static_cast<int>(vertices.size()) - 1);
			}
		}
	}
	m_navMesh->CreateNavMesh(vertices, mapWidth, mapHeight, vertexMapping);
	m_navMesh->CreateBuffers();

	// Validate Nav Mesh
	[[maybe_unused]] bool isNavMeshValid = m_navMesh->ValidateNavMesh();

// 	// Populate the map with props after generating initial NavMesh
// 	PopulateMapWithProps();
// 
// 	// Remove triangles that collide with props
// 	m_navMesh->RemoveTrianglesAffectedByProps(m_props);
}

void Map::Render() const
{
	RenderTerrain();
	RenderNavMesh();
	RenderProps();
	RenderActors();
}

void Map::RenderTerrain() const
{
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetDepthMode(DepthMode::ENABLED);
	g_theRenderer->SetRasterizerState(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->SetSamplerMode(SamplerMode::BILINEAR_WRAP);
	g_theRenderer->SetModelConstants();
	g_theRenderer->SetLightConstants(m_sunDirection.GetForwardVector(), m_sunIntensity, m_ambientIntensity, m_game->m_player->m_position, m_minFallOff, m_maxFallOff, m_minFallOffMultiplier, m_maxFallOffMultiplier, LightingDebug());
	
	if (m_grassDirtMat && m_rockMat && m_snowRockMat)
	{
 		g_theRenderer->BindTexture(0, m_grassDirtMat->m_diffuseTexture);
 		g_theRenderer->BindTexture(1, m_rockMat->m_diffuseTexture);
 		g_theRenderer->BindTexture(2, m_snowRockMat->m_diffuseTexture);

		g_theRenderer->BindTexture(3, m_grassDirtMat->m_normalTexture);
 		g_theRenderer->BindTexture(4, m_rockMat->m_normalTexture);
 		g_theRenderer->BindTexture(5, m_snowRockMat->m_normalTexture);

		g_theRenderer->BindTexture(6, m_grassDirtMat->m_specGlossEmitTexture);
		g_theRenderer->BindTexture(7, m_rockMat->m_specGlossEmitTexture);
		g_theRenderer->BindTexture(8, m_snowRockMat->m_specGlossEmitTexture);
		
		g_theRenderer->BindShader(m_terrainShader);
	}

	g_theRenderer->DrawVertexBufferIndex(m_terrainVertexBuffer, m_terrainIndexBuffer, VertexType::Vertex_PCUTBN, static_cast<int>(m_terrain->m_indices.size()));
}

void Map::RenderNavMesh() const
{
	if (m_navMesh)
	{
		if (m_game->m_enableNavMeshVisual)
		{
			m_navMesh->RenderNavMesh();
		}

		if (m_game->m_enableHeatmapVisual)
		{
			m_navMesh->RenderHeatMap();
		}

		if (m_game->m_enableBVHVisual)
		{
			m_navMesh->RenderBVH();
		}
	}
}

void Map::RenderProps() const
{
	if (!m_props.empty())
	{
		for (int index = 0; index < m_props.size(); index++)
		{
			if (m_props[index] == nullptr) continue;
			m_props[index]->Render();
		}
	}
}

void Map::RenderActors() const
{
	if (!m_agentActors.empty())
	{
		for (int index = 0; index < m_agentActors.size(); index++)
		{
			if (m_agentActors[index] == nullptr) continue;
			m_agentActors[index]->Render();
		}
	}
}

void Map::DebugRenderText() const
{
	std::string SunOrientationText = Stringf("Sun Orientation (ARROWS): (%.1f, %.1f, %.1f)", m_sunDirection.m_yawDegrees, m_sunDirection.m_pitchDegrees, m_sunDirection.m_rollDegrees);

	std::string SunDirectionText = Stringf("Sun Direction: (%.1f, %.1f, %.1f)", m_sunDirection.GetForwardVector(), m_sunDirection.GetLeftVector(), m_sunDirection.GetUpVector());
	std::string SunIntensityText = Stringf("Sun Intensity (< / >): %.1f", m_sunIntensity);
	std::string AmbientIntensityText = Stringf("Ambient Intensity (< / >): %.1f", m_ambientIntensity);

	DebugAddScreenText(SunOrientationText, Vec2(700.f, 650.f), 20.f, Vec2(0.5f, 0.5f), 0.f, Rgba8::WHITE, Rgba8::WHITE);

	DebugAddScreenText(SunDirectionText, Vec2(1000.f, 630.f), 20.f, Vec2(0.5f, 0.5f), 0.f, Rgba8::WHITE, Rgba8::WHITE);
	DebugAddScreenText(SunIntensityText, Vec2(1000.f, 610.f), 20.f, Vec2(0.5f, 0.5f), 0.f, Rgba8::WHITE, Rgba8::WHITE);
	DebugAddScreenText(AmbientIntensityText, Vec2(1000.f, 590.f), 20.f, Vec2(0.5f, 0.5f), 0.f, Rgba8::WHITE, Rgba8::WHITE);
}

void Map::PopulateMapWithProps()
{
	int numShapes = 4; // Prism, Pyramid, Cube, Cylinder
	int propsPerShape = m_numProps / numShapes;

	for (int i = 0; i < m_numProps; i++)
	{
		Vec3 randomPos = GetRandomPositionOnMap();
		if (randomPos == Vec3::ZERO) continue;

		Prop* newProp = new Prop(randomPos, EulerAngles::ZERO);
		newProp->m_color = newProp->m_color.GetRandomColor();

		// Distribute shapes evenly
		if (i < propsPerShape)
		{
			// Create Prism
			float randomLength = g_rng.SRollRandomFloatInRange(1.f, 2.f);
			float randomWidth = g_rng.SRollRandomFloatInRange(1.f, 2.f);
			float randomHeight = g_rng.SRollRandomFloatInRange(4.f, 8.f);
			newProp->CreatePrism(randomPos, randomLength, randomWidth, randomHeight);
		}
		else if (i < propsPerShape * 2)
		{
			// Create Pyramid
			float randomLength = g_rng.SRollRandomFloatInRange(1.f, 2.f);
			float randomWidth = g_rng.SRollRandomFloatInRange(1.f, 2.f);
			float randomHeight = g_rng.SRollRandomFloatInRange(4.f, 8.f);
			newProp->CreatePyramid(randomPos, randomLength, randomWidth, randomHeight);
		}
		else if (i < propsPerShape * 3)
		{
			// Create Cube
			float randomX = g_rng.SRollRandomFloatInRange(0.5f, 1.f);
			float randomY = g_rng.SRollRandomFloatInRange(0.5f, 1.f);
			float randomZ = g_rng.SRollRandomFloatInRange(0.5f, 1.f);
			Vec3 boxSize = Vec3(randomX, randomY, randomZ);
			newProp->CreateCube(boxSize);
		}
		else
		{
			// Create Cylinder
			Vec3 randomEndPos = randomPos - Vec3(0.f, 0.f, 0.2f);
			Vec3 randomStartPos = randomEndPos + Vec3(0.f, 0.f, g_rng.SRollRandomFloatInRange(1.f, 2.f));
			float randomRadiusSize = g_rng.SRollRandomFloatInRange(0.5f, 1.f);
			int randomNumSlices = g_rng.SRollRandomIntInRange(8, 32);
			newProp->CreateCylinder(randomStartPos, randomEndPos, randomRadiusSize, randomNumSlices);
		}

		// Create buffers for the prop
		newProp->CreateBuffers();
		m_props.emplace_back(newProp);
	}
}

Vec3 Map::GetRandomPositionOnMap()
{
	Vec3 randomPosition = Vec3::ZERO;

	if (m_navMesh)
	{
		// Get a random triangle from the navmesh
		int randomTriangleIndex = m_navMesh->GetRandomNavMeshTriangleIndex();

		if (randomTriangleIndex != -1)
		{
			// Get centroid of the triangle to place the prop
			randomPosition = m_navMesh->CalculateCentroid(m_navMesh->m_triangles[randomTriangleIndex]);
		}
	}

	return randomPosition;
}

void Map::PopulateMapWithAgentActors()
{
	std::vector<TempActorInfo> tempAgents; // Store temporary agents with pos and radius

	ActorDefinition* agentDef = ActorDefinition::GetActorDefByName("Agent");
	if (!agentDef)
	{
		ERROR_AND_DIE("Failed to get the agent actor definition");
	}
	
	for (int i = 0; i < m_maxNumAgents; i++)
	{
		bool validSpawnFound = false;
		Vec3 spawnPosition;
		float spawnRadius = 1.f;

		while (!validSpawnFound)
		{
			int randomTriangleIndex = m_navMesh->GetRandomNavMeshTriangleIndex();
			spawnPosition = m_navMesh->GetRandomPointInsideTriangle(randomTriangleIndex);

			validSpawnFound = true;
			for (TempActorInfo const& existingAgent : tempAgents)
			{
				float minDistance = spawnRadius + existingAgent.m_radius;
				float distSq = GetDistanceSquared3D(existingAgent.m_position, spawnPosition);
				if (distSq < minDistance)
				{
					validSpawnFound = false;
					break;
				}
			}
		}

		TempActorInfo newAgent;
		newAgent.m_position = spawnPosition;
		newAgent.m_radius = spawnRadius;
		tempAgents.emplace_back(newAgent);
	}

	for (TempActorInfo const& agentInfo : tempAgents)
	{
		MapSpawnInfo spawnInfo;
		spawnInfo.m_actorType = agentDef->m_name;
		spawnInfo.m_actorPosition = agentInfo.m_position;
		spawnInfo.m_actorOrientation = EulerAngles::ZERO;

		SpawnActor(spawnInfo);
	}
}

void Map::MapUpdate()
{
	[[maybe_unused]] float deltaSeconds = m_game->m_clock->GetDeltaSeconds();
	
	UpdateActors();
}

void Map::UpdateActors()
{
	if (!m_agentActors.empty())
	{
		for (int index = 0; index < m_agentActors.size(); index++)
		{
			if (m_agentActors[index] == nullptr) continue;
			m_agentActors[index]->Update();
		}
	}
}

std::vector<Prop*> Map::GetAllProps() const
{
	return m_props;
}

std::vector<Actor*> Map::GetAllAgents() const
{
	return m_agentActors;
}

ActorUID Map::GenerateActorUID(int actorIndex)
{
	ActorUID uid = ActorUID(m_actorSalt++, actorIndex);
	return uid;
}

Actor* Map::SpawnActor(const MapSpawnInfo& spawnInfo)
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

Actor* Map::GetActorByUID(const ActorUID uid) const
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

void Map::MapShutDown()
{
	SafeDelete(m_props);
	SafeDelete(m_aiPath);
	SafeDelete(m_navMesh);
	SafeDelete(m_terrain);
	SafeDelete(m_terrainShader);
	SafeDelete(m_grassDirtMat);
	SafeDelete(m_rockMat);
	SafeDelete(m_snowRockMat);
	SafeDelete(m_terrainVertexBuffer);
	SafeDelete(m_terrainIndexBuffer);
	SafeDelete(m_agentActors);
}
