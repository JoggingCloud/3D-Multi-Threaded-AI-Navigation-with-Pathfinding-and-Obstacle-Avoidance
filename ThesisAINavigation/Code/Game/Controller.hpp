#pragma once
#include "Game/GameCommon.hpp"
#include "Game/ActorUID.hpp"

//------------------------------------------------------------------------------------------------
class Actor;
class AIActor;
class Game;

//------------------------------------------------------------------------------------------------
class Controller
{
	friend class Actor;

public:
	Controller() = default;
	virtual ~Controller() = default;

	virtual void Update() = 0;

	void Possess(Actor* newActor);
	Actor* GetActor() const;

public:
	ActorUID m_actorUID = ActorUID::INVALID;
	Game* m_game = nullptr;
};