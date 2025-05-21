#pragma once
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
class Game;
class PlayerActor;
class Actor;
struct ActorUID;
class Shader;
class Texture;
class Image;
class VertexBuffer;
class IndexBuffer;
class NavMeshPathfinding;

constexpr float TILING_FACTOR = 5.f;
constexpr int NUM_REGIONS = 3;

struct MapConfig
{
	float m_groundHeightThreshold = 0.1f;
	float m_hillHeightTheshold = 0.6f;
	float m_mountainHeightThreshold = 1.f;

	int m_terrainWidthDimension = 20; // 300 is max size I can do before it crashes and its too slow to compute neighbors for nav mesh generation
	int m_terrainHeightDimension = 20; // 300 is max size I can do before it crashes and its too slow to compute neighbors for nav mesh generation 
};

struct TempActorInfo
{
	Vec3 m_position = Vec3::ZERO;
	float m_radius = 0.f;
};

struct MapSpawnInfo
{
	std::string m_actorType = "";
	Vec3 m_actorPosition;
	EulerAngles m_actorOrientation;
};

class Map
{
public:
	Map() = default;
	Map(Game* owner, MapConfig const& config, int numAgentsToSpawn);
	virtual ~Map();

	void LoadTerrainMaterial();
	void CreateTerrainBuffers();
	void GenerateNavMesh();

	void Render() const;
	void RenderTerrain() const;
	void RenderNavMesh() const;
	void RenderProps() const;
	void RenderActors() const;
	void DebugRenderText() const;

	void PopulateMapWithProps();
	Vec3 GetRandomPositionOnMap();

	inline int Get1DIndex(int x, int y, int width) const { return (y * width) + x; }

	void PopulateMapWithAgentActors();

	void MapUpdate();
	void UpdateActors();

	std::vector<Prop*> GetAllProps() const;
	std::vector<Actor*> GetAllAgents() const;
	ActorUID GenerateActorUID(int actorIndex);
	Actor* SpawnActor(const MapSpawnInfo& spawnInfo);
	Actor* GetActorByUID(const ActorUID uid) const;

	void MapShutDown();

public:
	Game* m_game = nullptr;
	MapConfig m_mapConfig;
	std::vector<MapSpawnInfo> m_spawnInfos;
	std::vector<Prop*> m_props;
	int m_numProps = 0/*12*/;
	
	Terrain*  m_terrain = nullptr;
	NavMesh*  m_navMesh = nullptr;
	NavMeshPathfinding* m_aiPath = nullptr;
	Material* m_rockMat = nullptr;
	Material* m_snowRockMat = nullptr;
	Material* m_grassDirtMat = nullptr;

public:
	float m_maxTraversalDistance = 22.f;

	Texture* m_terrainTexture = nullptr;
	Shader* m_terrainShader = nullptr;

	VertexBuffer* m_terrainVertexBuffer = nullptr;
	IndexBuffer* m_terrainIndexBuffer = nullptr;

public:
	std::vector<Actor*> m_agentActors;
	int m_maxNumAgents = 0;
	int m_agentID = 0;
	static const unsigned int MAX_ACTOR_SALT = 0x0000fffeu;
	unsigned int m_actorSalt = MAX_ACTOR_SALT;

public:
	bool m_canSeePlaneBounds = false;
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
