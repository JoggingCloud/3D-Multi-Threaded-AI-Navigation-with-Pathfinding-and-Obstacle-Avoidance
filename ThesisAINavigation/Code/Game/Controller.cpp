#include "Game/Controller.hpp"
#include "Game/Actor.hpp"
#include "Game/Game.hpp"
#include "Game/App.hpp"
#include "Game/PlayerActor.hpp"
#include "Game/Map.hpp"

void Controller::Possess(Actor* newActor)
{
	Actor* currentActor = GetActor();
	if (currentActor != nullptr)
	{
		currentActor->OnUnPossessed(this);
	}

	if (newActor == nullptr || !newActor->m_canBePossessed)
	{
		return;
	}

	// Set the new actor
	m_actorUID = newActor->GetUID();
	newActor->OnPossessed(this);
}

Actor* Controller::GetActor() const
{
	if (m_actorUID == ActorUID::INVALID)
	{
		return nullptr;
	}

	Actor* currentPossessedActor = g_theApp->m_currentGame->m_map->GetActorByUID(m_actorUID);
	return currentPossessedActor;
}