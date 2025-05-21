#pragma once // Means to not copy this file more than once. Every header file will have this 
#include "Engine/Core/EventSystem.hpp"
#include <cstdlib>
#include <bitset>

class Game;
class Clock;

enum class GameModeType
{
	ASTAR_SINGLE_AGENT_MODE,
	ASTAR_MULTIAGENT_MODE,
	
	VO_TWOAGENTS_MODE,
	VO_THREEAGENTS_MODE, // Show flaws
	
	RVO_TWO_AGENTS_MODE,
	RVO_THREE_AGENTS_MODE, // Show possible flaws here
	
	HRVO_TWO_AGENTS_MODE,
	HRVO_THREE_AGENTS_MODE, // Show possible flaws here

	ORCA_TWO_AGENTS_MODE,
	ORCA_THREE_AGENTS_MODE,
	ORCA_LARGESCALE_MULTIAGENT_MODE,
	ORCA_BOTTLENECK_MODE, // Show congestion issues
	
	ASTAR_ORCA_MINI_AGENT_MODE,
	ASTAR_ORCA_MULTIAGENT_MODE,

	NUM_GAME_MODES
};

class App
{
public:
	Game* m_currentGame = nullptr;
	GameModeType m_currentGameModeType = GameModeType::ASTAR_SINGLE_AGENT_MODE;

public:
	App() = default; // Construction
	~App() = default; // Destruction
	void Startup();
	void Shutdown();
	void RunFrame();
	void Run();

	void LoadGameData();
	bool IsQuitting() const { return m_isQuitting; }
	bool HandleQuitRequested();

	static bool Event_ResetGame(EventArgs& args);
	static bool Event_Quit(EventArgs& args);
	static bool Event_WindowMinimized(EventArgs& args);
	static bool Event_WindowMaximized(EventArgs& args);
	static bool Event_WindowRestored(EventArgs& args);
	static bool Event_GameModeSelection(EventArgs& args);

	static bool Command_DisplayGameModes(EventArgs& args);

	static std::string GetGameModeNameByString(GameModeType mode);

private:
	void BeginFrame();
	void ResetCurrentGameMode();
	void Update();
	void UpdateGameMode();
	void SwitchToNewGameMode(GameModeType newGameModeType);
	void DisplayAllGameModes();
	void Render();
	void EndFrame();

private:
	Clock *m_clock = nullptr;
	bool m_isQuitting = false;
	bool m_isPaused = false;
};