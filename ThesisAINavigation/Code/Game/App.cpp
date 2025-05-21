#include "Game/App.hpp"
#include "Game/Game.hpp"
#include "Game/GameModes/NavigationMode.hpp"
#include "Game/GameModes/ObstacleAvoidanceMode.hpp"

#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/Window.hpp"
#include "Engine/Renderer/DebugRenderer.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/DevConsole.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Core/Time.hpp"
#include "Engine/Math/MathUtils.hpp"

#include "ThirdParty/TinyXML2/tinyxml2.h"

Window*		 g_theWindow   = nullptr;
App*		 g_theApp      = nullptr;
Renderer*	 g_theRenderer = nullptr;
InputSystem* g_theInput    = nullptr;
AudioSystem* g_theAudio    = nullptr;

bool App::Event_ResetGame([[maybe_unused]] EventArgs& args)
{
	g_theApp->ResetCurrentGameMode();
	return true;
}

bool App::Event_Quit([[maybe_unused]] EventArgs& args)
{
	g_theApp->HandleQuitRequested();
	return false;
}

bool App::Event_WindowMinimized([[maybe_unused]] EventArgs& args)
{
	g_theApp->m_isPaused = true;
	return false;
}

bool App::Event_WindowMaximized([[maybe_unused]] EventArgs& args)
{
	return false;
}

bool App::Event_WindowRestored([[maybe_unused]] EventArgs& args)
{
	g_theApp->m_isPaused = false;
	return false;
}

bool App::Event_GameModeSelection(EventArgs& args)
{
	int mode = std::stoi(args.GetValue<std::string>("mode", ""));

	if (mode >= 0 && mode < static_cast<int>(GameModeType::NUM_GAME_MODES))
	{
		g_theApp->SwitchToNewGameMode(static_cast<GameModeType>(mode));
		return true;
	}
	return false;
}

bool App::Command_DisplayGameModes([[maybe_unused]] EventArgs& args)
{
	g_theConsole->AddLine(DevConsole::INFO_MAJOR, "---------------------------");
	g_theConsole->AddLine(DevConsole::INFO_MINOR, "List of Game Modes!");
	g_theConsole->AddLine(DevConsole::INFO_MAJOR, "---------------------------");

	g_theApp->DisplayAllGameModes();

	g_theConsole->AddLine(DevConsole::INFO_MAJOR, "---------------------------");

	return false;
}

void App::Startup()
{
	LoadGameData();

	JobSystemConfig jobSystemConfig;
	jobSystemConfig.m_numWorkers = -1;
	g_theJobSystem = new JobSystem(jobSystemConfig);

	EventSystemConfig eventConfig;
	g_theEventSystem = new EventSystem(eventConfig);

	InputConfig inputConfig;
	g_theInput = new InputSystem(inputConfig);

	WindowConfig windowConfig;
	windowConfig.m_inputSystem = g_theInput;
	windowConfig.m_windowTitle = g_defaultConfigBlackboard->GetValue("windowTitle", "Unnamed Application");
	windowConfig.m_aspectRatio = g_defaultConfigBlackboard->GetValue("windowAspect", 2.f);
	windowConfig.m_fullScreen = g_defaultConfigBlackboard->GetValue("windowFullscreen", false);
	windowConfig.m_size = g_defaultConfigBlackboard->GetValue("windowSize", IntVec2(1500, 750));
	windowConfig.m_pos = g_defaultConfigBlackboard->GetValue("windowPosition", IntVec2(75, 150));
	g_theWindow = new Window(windowConfig);

	RenderConfig renderConfig;
	renderConfig.m_window = g_theWindow;
	g_theRenderer = new Renderer(renderConfig);

	DevConsoleConfig devConsoleConfig;
	devConsoleConfig.m_filePath = "Data/Fonts/SquirrelFixedFont.png";
	g_theConsole = new DevConsole(devConsoleConfig);
	g_theConsole->SetMode(DevConsoleMode::HIDDEN);

	AudioConfig audioConfig;
	g_theAudio = new AudioSystem(audioConfig);

	DebugRenderConfig debugConfig;
	debugConfig.m_renderer = g_theRenderer;

	GameModeConfig aStarGameSingleConfig;
	aStarGameSingleConfig.m_useAStar = true;
	aStarGameSingleConfig.m_terrianDimensions = IntVec2(20, 20);
	aStarGameSingleConfig.m_groundHeight = 0.3f;
	aStarGameSingleConfig.m_hillHeight = 0.6f;
	aStarGameSingleConfig.m_mountainHeight = 1.f;
	aStarGameSingleConfig.m_numberOfAgents = 1;
	m_currentGame = new NavigationMode(aStarGameSingleConfig);

	g_theJobSystem->Startup();
	g_theEventSystem->StartUp();
	SubscribeEventCallbackFunction("ResetGame", App::Event_ResetGame);
	SubscribeEventCallbackFunction("Quit", App::Event_Quit);
	SubscribeEventCallbackFunction("WindowMinimized", App::Event_WindowMinimized);
	SubscribeEventCallbackFunction("WindowMaximized", App::Event_WindowMaximized);
	SubscribeEventCallbackFunction("WindowRestored", App::Event_WindowRestored);
	SubscribeEventCallbackFunction("GameModeSelection", App::Event_GameModeSelection);
	SubscribeEventCallbackFunction("DisplayGameModes", App::Command_DisplayGameModes);

	g_theConsole->Startup();
	g_theInput->Startup();
	g_theWindow->Startup();
	g_theRenderer->Startup();
	g_theAudio->Startup();
	m_currentGame->Startup();
	Clock::TickSystemClock();

	DebugRenderSystemStartup(debugConfig);
}

void App::Shutdown()
{
	m_currentGame->Shutdown();

	DebugRenderSystemShutdown();
	g_theAudio->Shutdown();
	g_theConsole->ShutDown();
	g_theRenderer->Shutdown();
	g_theWindow->Shutdown();
	g_theInput->Shutdown();
	g_theEventSystem->ShutDown();
	g_theJobSystem->ShutDown();

	SafeDelete(g_theAudio);
	SafeDelete(g_theConsole);
	SafeDelete(g_theRenderer);
	SafeDelete(g_theWindow);
	SafeDelete(g_theInput);
	SafeDelete(g_theEventSystem);
	SafeDelete(g_theJobSystem);
}

void App::RunFrame()
{
	BeginFrame();

	if (!m_isPaused)
	{
		Update();
		Render();
	}

	EndFrame();
}

void App::Run()
{
	// program main loop; keep running frames until it's time to quit
	while (!m_isQuitting)
	{
		RunFrame();
	}
}

void App::LoadGameData()
{
	tinyxml2::XMLDocument document;

	g_defaultConfigBlackboard = new NamedStrings();

	// Load and parse GameConfig.xml
	if (document.LoadFile("Data/GameConfig.xml") == tinyxml2::XML_SUCCESS)
	{
		// Get the root element 
		tinyxml2::XMLElement* rootElement = document.RootElement();
		GUARANTEE_OR_DIE(rootElement, "XML couldn't be loaded"); // if files doesn't exist then print this text

		// Populate g_gameConfigBlackboard with attributes
		g_defaultConfigBlackboard->PopulateFromXmlElementAttributes(*rootElement, false);
	}
}

bool App::HandleQuitRequested()
{
	m_isQuitting = true;
	return false;
}

void App::BeginFrame()
{
	g_theJobSystem->BeginFrame();
	g_theEventSystem->BeginFrame();
	g_theInput->BeginFrame();
	g_theWindow->BeginFrame();
	g_theRenderer->BeginFrame();
	g_theConsole->BeginFrame();
	g_theAudio->BeginFrame();
	Clock::TickSystemClock();
	DebugRenderBeginFrame();
}

void App::ResetCurrentGameMode()
{
	if (m_currentGame)
	{
		m_currentGame->Shutdown();
		SafeDelete(m_currentGame);
	}
	SwitchToNewGameMode(m_currentGameModeType);
}

void App::Update()
{
	m_currentGame->UpdateGameMode();
	if (g_theInput->WasKeyJustPressed('R'))
	{
		ResetCurrentGameMode();
	}
	UpdateGameMode();
}

void App::UpdateGameMode()
{
	if (g_theInput->WasKeyJustPressed(KEYCODE_RIGHTARROW)) 	// Go right 
	{
		if ((int)m_currentGameModeType + 1 == (int)GameModeType::NUM_GAME_MODES)
		{
			SwitchToNewGameMode(static_cast<GameModeType>(GameModeType::ASTAR_SINGLE_AGENT_MODE));
		}
		else
		{
			SwitchToNewGameMode(static_cast<GameModeType>(((int)m_currentGameModeType + 1)));
		}
	}
	else if (g_theInput->WasKeyJustPressed(KEYCODE_LEFTARROW)) // Go left 
	{
		if ((int)m_currentGameModeType - 1 < 0)
		{
			SwitchToNewGameMode(static_cast<GameModeType>((int)GameModeType::NUM_GAME_MODES - 1));
		}
		else
		{
			SwitchToNewGameMode(static_cast<GameModeType>(((int)m_currentGameModeType - 1)));
		}
	}
}

void App::SwitchToNewGameMode(GameModeType newGameModeType)
{
	if (m_currentGame)
	{
		m_currentGame->Shutdown();
		SafeDelete(m_currentGame);
	}

	m_currentGameModeType = newGameModeType;

	switch (m_currentGameModeType)
	{
	case GameModeType::ASTAR_SINGLE_AGENT_MODE:
	{
		GameModeConfig aStarGameSingleConfig;
		aStarGameSingleConfig.m_useAStar = true;
		aStarGameSingleConfig.m_terrianDimensions = IntVec2(20, 20);
		aStarGameSingleConfig.m_groundHeight = 0.3f;
		aStarGameSingleConfig.m_hillHeight = 0.6f;
		aStarGameSingleConfig.m_mountainHeight = 1.f;
		aStarGameSingleConfig.m_numberOfAgents = 1;
		m_currentGame = new NavigationMode(aStarGameSingleConfig);
		break;
	}
	case GameModeType::ASTAR_MULTIAGENT_MODE:
	{
		GameModeConfig aStarGameMultiConfig;
		aStarGameMultiConfig.m_useAStar = true;
		aStarGameMultiConfig.m_terrianDimensions = IntVec2(80, 80);
		aStarGameMultiConfig.m_groundHeight = 0.3f;
		aStarGameMultiConfig.m_hillHeight = 0.6f;
		aStarGameMultiConfig.m_mountainHeight = 1.f;
		aStarGameMultiConfig.m_numberOfAgents = 80;
		m_currentGame = new NavigationMode(aStarGameMultiConfig);
		break;
	}
	case GameModeType::VO_TWOAGENTS_MODE:
	{
		GameModeConfig voTwoGameConfig;
		voTwoGameConfig.m_useVO = true;
		voTwoGameConfig.m_numberOfAgents = 2;
		m_currentGame = new ObstacleAvoidanceMode(voTwoGameConfig);
		break;
	}
	case GameModeType::VO_THREEAGENTS_MODE:
	{
		GameModeConfig voThreeGameConfig;
		voThreeGameConfig.m_useVO = true;
		voThreeGameConfig.m_numberOfAgents = 3;
		m_currentGame = new ObstacleAvoidanceMode(voThreeGameConfig);
		break;
	}
	case GameModeType::RVO_TWO_AGENTS_MODE:
	{
		GameModeConfig rvoTwoGameConfig;
		rvoTwoGameConfig.m_useRVO = true;
		rvoTwoGameConfig.m_numberOfAgents = 2;
		m_currentGame = new ObstacleAvoidanceMode(rvoTwoGameConfig);
		break;
	}
	case GameModeType::RVO_THREE_AGENTS_MODE:
	{
		GameModeConfig rvoThreeGameConfig;
		rvoThreeGameConfig.m_useRVO = true;
		rvoThreeGameConfig.m_numberOfAgents = 3;
		m_currentGame = new ObstacleAvoidanceMode(rvoThreeGameConfig);
		break;
	}
	case GameModeType::HRVO_TWO_AGENTS_MODE:
	{
		GameModeConfig hrvoTwoGameConfig;
		hrvoTwoGameConfig.m_useHRVO = true;
		hrvoTwoGameConfig.m_numberOfAgents = 2;
		m_currentGame = new ObstacleAvoidanceMode(hrvoTwoGameConfig);
		break;
	}
	case GameModeType::HRVO_THREE_AGENTS_MODE:
	{
		GameModeConfig hrvoThreeGameConfig;
		hrvoThreeGameConfig.m_useHRVO = true;
		hrvoThreeGameConfig.m_numberOfAgents = 3;
		m_currentGame = new ObstacleAvoidanceMode(hrvoThreeGameConfig);
		break;
	}
	case GameModeType::ORCA_TWO_AGENTS_MODE:
	{
		GameModeConfig orcaTwoGameConfig;
		orcaTwoGameConfig.m_useORCA = true;
		orcaTwoGameConfig.m_numberOfAgents = 2;
		m_currentGame = new ObstacleAvoidanceMode(orcaTwoGameConfig);
		break;
	}
	case GameModeType::ORCA_THREE_AGENTS_MODE:
	{
		GameModeConfig orcaThreeGameConfig;
		orcaThreeGameConfig.m_useORCA = true;
		orcaThreeGameConfig.m_numberOfAgents = 3;
		m_currentGame = new ObstacleAvoidanceMode(orcaThreeGameConfig);
		break;
	}
	case GameModeType::ORCA_LARGESCALE_MULTIAGENT_MODE:
	{
		GameModeConfig orcaLargeScaleGameConfig;
		orcaLargeScaleGameConfig.m_useORCA = true;
		orcaLargeScaleGameConfig.m_numberOfAgents = 10; 
		m_currentGame = new ObstacleAvoidanceMode(orcaLargeScaleGameConfig);
		break;
	}
	case GameModeType::ORCA_BOTTLENECK_MODE:
	{
		GameModeConfig orcaBottleNeckGameConfig;
		orcaBottleNeckGameConfig.m_useORCA = true;
		orcaBottleNeckGameConfig.m_numberOfAgents = 20;
		m_currentGame = new ObstacleAvoidanceMode(orcaBottleNeckGameConfig);
		break;
	}
	case GameModeType::ASTAR_ORCA_MINI_AGENT_MODE:
	{
		GameModeConfig astarORCATwoGameConfig;
		astarORCATwoGameConfig.m_useAStar = true;
		astarORCATwoGameConfig.m_useORCA = true;
		astarORCATwoGameConfig.m_terrianDimensions = IntVec2(20, 20);
		astarORCATwoGameConfig.m_groundHeight = 0.3f;
		astarORCATwoGameConfig.m_hillHeight = 0.6f;
		astarORCATwoGameConfig.m_mountainHeight = 1.f;
		astarORCATwoGameConfig.m_numberOfAgents = 15;
		m_currentGame = new NavigationMode(astarORCATwoGameConfig);
		break;
	}
	case GameModeType::ASTAR_ORCA_MULTIAGENT_MODE: 
	{
		GameModeConfig astarORCAMultiGameConfig;
		astarORCAMultiGameConfig.m_useAStar = true;
		astarORCAMultiGameConfig.m_useORCA = true;
		astarORCAMultiGameConfig.m_terrianDimensions = IntVec2(80, 80);
		astarORCAMultiGameConfig.m_groundHeight = 0.3f;
		astarORCAMultiGameConfig.m_hillHeight = 0.6f;
		astarORCAMultiGameConfig.m_mountainHeight = 1.f;
		astarORCAMultiGameConfig.m_numberOfAgents = 75;
		m_currentGame = new NavigationMode(astarORCAMultiGameConfig);
		break;
	}
	}
	m_currentGame->Startup();
}

std::string App::GetGameModeNameByString(GameModeType mode)
{
	switch (mode)
	{
	case GameModeType::ASTAR_SINGLE_AGENT_MODE:         return "ASTAR_SINGLE_AGENT_MODE";
	case GameModeType::ASTAR_MULTIAGENT_MODE:           return "ASTAR_MULTIAGENT_MODE";
	case GameModeType::VO_TWOAGENTS_MODE:               return "VO_TWOAGENTS_MODE";
	case GameModeType::VO_THREEAGENTS_MODE:             return "VO_THREEAGENTS_MODE";
	case GameModeType::RVO_TWO_AGENTS_MODE:             return "RVO_TWO_AGENTS_MODE";
	case GameModeType::RVO_THREE_AGENTS_MODE:           return "RVO_THREE_AGENTS_MODE";
	case GameModeType::HRVO_TWO_AGENTS_MODE:            return "HRVO_TWO_AGENTS_MODE";
	case GameModeType::HRVO_THREE_AGENTS_MODE:          return "HRVO_THREE_AGENTS_MODE";
	case GameModeType::ORCA_TWO_AGENTS_MODE:            return "ORCA_TWO_AGENTS_MODE";
	case GameModeType::ORCA_THREE_AGENTS_MODE:          return "ORCA_THREE_AGENTS_MODE";
	case GameModeType::ORCA_LARGESCALE_MULTIAGENT_MODE: return "ORCA_LARGESCALE_MULTIAGENT_MODE";
	case GameModeType::ORCA_BOTTLENECK_MODE:            return "ORCA_BOTTLENECK_MODE";
	case GameModeType::ASTAR_ORCA_MINI_AGENT_MODE:       return "ASTAR_ORCA_MINI_AGENT_MODE";
	case GameModeType::ASTAR_ORCA_MULTIAGENT_MODE:      return "ASTAR_ORCA_MULTIAGENT_MODE";
	default:                                            return "Unknown Mode";
	}
}

void App::DisplayAllGameModes()
{
	for (int i = 0; i < static_cast<int>(GameModeType::NUM_GAME_MODES); i++)
	{
		GameModeType mode = static_cast<GameModeType>(i);
		std::string modeName = GetGameModeNameByString(mode);
		g_theConsole->AddLine(DevConsole::INFO_MAJOR, "Mode " + std::to_string(i) + ": " + modeName);
	}
}

void App::Render()
{
	g_theRenderer->ClearScreen(Rgba8::LIGHT_GRAY);
	m_currentGame->Render();
	g_theConsole->Render(AABB2(Vec2(0.25f, 0.f), Vec2((float)g_theWindow->GetClientDimensions().x, (float)g_theWindow->GetClientDimensions().y)), g_theRenderer);
}

void App::EndFrame()
{
	DebugRenderEndFrame();
	g_theJobSystem->EndFrame();
	g_theEventSystem->EndFrame();
	g_theAudio->EndFrame();
	g_theConsole->EndFrame();
	g_theRenderer->EndFrame();
	g_theInput->EndFrame();
	g_theWindow->EndFrame();
}
