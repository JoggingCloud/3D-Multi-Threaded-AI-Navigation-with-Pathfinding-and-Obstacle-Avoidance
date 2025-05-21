#pragma once
#include "Engine/Renderer/Camera.hpp"
#include "Game/Controller.hpp"
#include "Engine/Math/Mat44.hpp"

class Camera;
class Clock;
class Game;
class Actor;

class PlayerActor : public Controller 
{
public:
	PlayerActor(Game* owner, Vec3 const& startPos, EulerAngles const& startOrientation);
	PlayerActor() = default;
	virtual ~PlayerActor() {};
	virtual void Update();

	void FreeFlyMouseMovementUpdate();
	void FreeFlyKeyInputUpdate();

	void DebugKeys();

public:
	Mat44 GetModelMatrix() const;

public:
	Game* m_game = nullptr;
	Vec3 m_position = Vec3::ZERO;
	EulerAngles m_orientation = EulerAngles::ZERO;
	
	Camera m_playerWorldView;
	Camera m_playerUICamera;

	float m_movementSpeed = 20.f;
	float m_controlledMovementSpeed = 0.f;
	float m_camerAspectRatio = 2.f;

	bool m_debugMouseEnabled = false;

	ActorUID m_targetActorUID = ActorUID::INVALID;
};