#include "Game/PlayerActor.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Core/DevConsole.hpp"
#include "Engine/Core/Clock.hpp"
#include "Game/Game.hpp"
#include "Game/Map.hpp"
#include "Game/App.hpp"
#include "Game/Actor.hpp"
#include "Engine/Core/Clock.hpp"
#include "Game/ActorDefinitions.hpp"
#include "Engine/Renderer/DebugRenderer.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Core/VertexUtils.hpp"

extern Renderer* g_theRenderer;
extern InputSystem* g_theInput;

PlayerActor::PlayerActor(Game* owner, Vec3 const& startPos, EulerAngles const& startOrientation)
	: m_game(owner), m_position(startPos), m_orientation(startOrientation)
{
}

void PlayerActor::Update()
{
	DebugKeys();

	FreeFlyMouseMovementUpdate();
	FreeFlyKeyInputUpdate();

	m_playerWorldView.SetPerspectiveView(2.f, 60.f, 0.1f, 10000.f);
	m_playerWorldView.SetRenderBasis(Vec3(0.f, 0.f, 1.f), Vec3(-1.f, 0.f, 0.f), Vec3(0.f, 1.f, 0.f));
	m_playerWorldView.SetTransform(m_position, m_orientation);
}

void PlayerActor::FreeFlyMouseMovementUpdate()
{
	m_orientation.m_pitchDegrees = GetClamped(m_orientation.m_pitchDegrees, -85.f, 85.f);
	m_orientation.m_yawDegrees -= g_theInput->GetCursorClientDelta().x * 0.075f;
	m_orientation.m_pitchDegrees += GetClamped(g_theInput->GetCursorClientDelta().y * 0.075f, -85.f, 85.f);
}

void PlayerActor::FreeFlyKeyInputUpdate()
{
	Vec3 forward = m_orientation.GetForwardVector();
	Vec3 right = m_orientation.GetRightVector();
	Vec3 up = m_orientation.GetUpVector();

	if (g_theInput->IsKeyDown(KEYCODE_SHIFT))
	{
		m_movementSpeed = 5.f;
	}
	else
	{
		m_movementSpeed = 2.f;
	}

	if (g_theInput->WasKeyJustPressed('H'))
	{
		m_position = Vec3(0.f, 0.f, 0.f);
		m_orientation = EulerAngles::ZERO;
	}

	float movementStep = m_movementSpeed * 0.01f; // Fixed step independent of deltaSeconds

	if (g_theInput->IsKeyDown('W')) // Move forward
	{
		m_position += forward * movementStep;
	}

	if (g_theInput->IsKeyDown('S')) // Move backward
	{
		m_position -= forward * movementStep;
	}

	if (g_theInput->IsKeyDown('A')) // Move left
	{
		m_position -= right * movementStep;
	}

	if (g_theInput->IsKeyDown('D')) // Move right
	{
		m_position += right * movementStep;
	}

	if (g_theInput->IsKeyDown('Q')) // Move up
	{
		m_position += up * movementStep;
	}

	if (g_theInput->IsKeyDown('E')) // Move down
	{
		m_position -= up * movementStep;
	}
}

void PlayerActor::DebugKeys()
{
	if (g_theConsole->IsOpen())
	{
		g_theInput->SetCursorMode(false, false);
	}
	else if (!m_debugMouseEnabled)
	{
		g_theInput->SetCursorMode(true, true);
	}
	
	if (g_theInput->WasKeyJustPressed('P'))
	{
		m_game->m_clock->TogglePause();
	}

	if (g_theInput->WasKeyJustPressed('O'))
	{
		m_game->m_clock->StepSingleFrame();
	}

	if (!g_theConsole->IsOpen() && g_theInput->WasKeyJustPressed(KEYCODE_ESC))
	{
		g_theApp->HandleQuitRequested();
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_F11))
	{
		m_debugMouseEnabled = !m_debugMouseEnabled;

		if (m_debugMouseEnabled)
		{
			g_theInput->SetCursorMode(false, false);
		}
		else
		{
			g_theInput->SetCursorMode(true, true);
		}
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_F1))
	{
		m_game->m_enableNavMeshVisual = !m_game->m_enableNavMeshVisual;
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_F2))
	{
		m_game->m_enablePathVisual = !m_game->m_enablePathVisual;
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_F3))
	{
		m_game->m_enableHeatmapVisual = !m_game->m_enableHeatmapVisual;
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_F4))
	{
		m_game->m_enableFOVZoneVisual = !m_game->m_enableFOVZoneVisual;
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_F5))
	{
		m_game->m_enableCurrentVOAlgorithmVisual = !m_game->m_enableCurrentVOAlgorithmVisual;
	}
}

Mat44 PlayerActor::GetModelMatrix() const
{
	Mat44 translation = Mat44::CreateTranslation3D(m_position);
	Mat44 orientation = m_orientation.GetAsMatrix_IFwd_JLeft_KUp();

	translation.Append(orientation);
	return translation;
}
