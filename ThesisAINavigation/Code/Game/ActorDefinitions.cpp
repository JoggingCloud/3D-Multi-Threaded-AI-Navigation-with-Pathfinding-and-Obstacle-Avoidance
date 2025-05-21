#include "Game/ActorDefinitions.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/Shader.hpp"
#include "Game/GameCommon.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include <d3d11.h>

ActorDefinition::ActorDefinition(const tinyxml2::XMLElement* element)
{
	m_name = ParseXmlAttribute(*element, "name", std::string());
	m_canBePossed = ParseXmlAttribute(*element, "canBePossessed", false);
	m_visible = ParseXmlAttribute(*element, "visible", false);
}

void ActorDefinition::InitializeActorDef()
{
	tinyxml2::XMLDocument doc;
	if (doc.LoadFile("Data/Definitions/ActorDefinitions.xml") != tinyxml2::XML_SUCCESS)
	{
		// Handle error loading XML file
		return;
	}

	const tinyxml2::XMLElement* root = doc.FirstChildElement("Definitions");
	if (!root)
	{
		// Handle missing root element
		ERROR_AND_DIE("Failed to get the root in the actor definition xml file");
		return;
	}

	if (s_actorDefinition.empty())
	{
		const tinyxml2::XMLElement* actorElement = root->FirstChildElement("ActorDefinition");
		if (actorElement)
		{
			ActorDefinition* newActorDef = new ActorDefinition(actorElement);

			// Parse Collision element
			const tinyxml2::XMLElement* collisionElement = actorElement->FirstChildElement("Collision");
			if (collisionElement)
			{
				newActorDef->m_radius = ParseXmlAttribute(*collisionElement, "radius", FloatRange::ZERO);
				newActorDef->m_height = ParseXmlAttribute(*collisionElement, "height", 0.f);
			}

			// Parse Physics element
			const tinyxml2::XMLElement* physicsElement = actorElement->FirstChildElement("Physics");
			if (physicsElement)
			{
				newActorDef->m_simulated = ParseXmlAttribute(*physicsElement, "simulated", false);
				newActorDef->m_moveSpeed = ParseXmlAttribute(*physicsElement, "moveSpeed", FloatRange::ZERO);
				newActorDef->m_turnSpeed = ParseXmlAttribute(*physicsElement, "turnSpeed", 0.f);
				newActorDef->m_drag = ParseXmlAttribute(*physicsElement, "drag", 0.f);
			}

			// Parse Camera element
			const tinyxml2::XMLElement* cameraElement = actorElement->FirstChildElement("Camera");
			if (cameraElement)
			{
				newActorDef->m_eyeHeight = ParseXmlAttribute(*cameraElement, "eyeHeight", 0.f);
			}

			// Parse AI element
			const tinyxml2::XMLElement* aiElement = actorElement->FirstChildElement("AI");
			if (aiElement)
			{
				newActorDef->m_aiEnabled = ParseXmlAttribute(*aiElement, "aiEnabled", false);
			}

			s_actorDefinition.push_back(newActorDef);
		}
	}
}

ActorDefinition* ActorDefinition::GetActorDefByName(const std::string& name)
{
	for (int i = 0; i < s_actorDefinition.size(); i++)
	{
		if (s_actorDefinition[i]->m_name == name)
		{
			return s_actorDefinition[i];
		}
	}
	return nullptr;
}

ActorDefinitions::ActorDefinitions()
{
}

ActorDefinitions::~ActorDefinitions()
{
	s_actorDefinition.clear();
}
