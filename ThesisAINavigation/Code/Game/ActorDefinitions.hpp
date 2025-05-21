#pragma once
#include "Engine/Core/XmlUtils.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include <vector>

class SpriteSheet;
class Image;
class Shader;

struct ActorDefinition
{
	explicit ActorDefinition(const tinyxml2::XMLElement* element);

	std::string m_name = "";
	std::string m_faction = "";

	bool m_canBePossed = false;
	bool m_visible = false;

	// Physics
	FloatRange m_radius = FloatRange::ZERO;
	FloatRange m_moveSpeed = FloatRange::ZERO;
	float m_turnSpeed = 0.f;
	float m_height = 0.f;
	bool m_simulated = false;
	float m_drag = 0.f;

	// Camera
	float m_eyeHeight = 0.f;

	// Ai
	bool m_aiEnabled = false;

	static void InitializeActorDef();
	static ActorDefinition* GetActorDefByName(const std::string& name);
};
static std::vector<ActorDefinition*> s_actorDefinition;

class ActorDefinitions
{
public:
	ActorDefinitions();
	~ActorDefinitions();
};