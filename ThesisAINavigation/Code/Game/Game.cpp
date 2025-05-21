#include "Game/Game.hpp"
#include "Game/App.hpp"

#include "Engine/Renderer/DebugRenderer.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/Texture.hpp"
#include "Engine/Renderer/Window.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/SimpleTriangleFont.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Core/DevConsole.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Input/InputSystem.hpp"

#include <vector>

extern DevConsole* g_theConsole;

Game::Game(GameModeConfig const& config)
	: m_gameModeConfig(config)
{
	m_clock = new Clock();
	m_clock->Pause();
}

void Game::CreateSky()
{
	m_skyBoxTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/skybox.png");
	AddVertsForZSphere(m_skyVertices, m_skyIndexes, Vec3::ZERO, 1000.f, Rgba8::WHITE, AABB2::ZERO_TO_ONE, 128);
}

void Game::CreateSkyBuffers()
{
	m_skyVertexBuffer = g_theRenderer->CreateVertexBuffer(m_skyVertices.size());
	g_theRenderer->CopyCPUToGPU(m_skyVertices.data(), m_skyVertices.size() * sizeof(Vertex_PCU), m_skyVertexBuffer);

	m_skyIndexBuffer = g_theRenderer->CreateIndexBuffer(m_skyIndexes.size());
	g_theRenderer->CopyCPUToGPU(m_skyIndexes.data(), m_skyIndexes.size() * sizeof(unsigned int), m_skyIndexBuffer);
}

void Game::FPSCalculation()
{
	float deltaTime = m_clock->GetDeltaSeconds();
	m_frameTimes.push_back(deltaTime);
	if (m_frameTimes.size() > m_maxFrameSamples)
	{
		m_frameTimes.pop_front();
	}

	// FPS Update Timer
	m_fpsUpdateTimer += deltaTime;
	if (m_fpsUpdateTimer >= 0.7f) // Update every 0.7 seconds
	{
		m_displayedFps = GetGameFPS();
		m_displayedFpsHistory.emplace_back(m_displayedFps);
		if (m_displayedFpsHistory.size() > m_maxFrameSamples)
		{
			m_displayedFpsHistory.pop_front();
		}
		m_fpsUpdateTimer = 0.f;
	}
}

void Game::PrintAverageFPS() const
{
	if (m_displayedFpsHistory.empty())
	{
		DebuggerPrintf("No FPS data to average.\n");
		return;
	}

	float totalFPS = 0.0f;
	for (float fps : m_displayedFpsHistory)
	{
		totalFPS += fps;
	}

	float averageFPS = totalFPS / static_cast<float>(m_displayedFpsHistory.size());

	DebuggerPrintf("=== FPS Test Result ===\n");
	DebuggerPrintf("Average FPS: %.2f\n", averageFPS);
}

void Game::RunFrame()
{
	Render();
/*	EndFrame();*/
}

void Game::RenderSkyBox() const
{
	g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
	g_theRenderer->SetDepthMode(DepthMode::ENABLED);
	g_theRenderer->SetRasterizerState(RasterizerMode::SOLID_CULL_NONE);
	g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
	g_theRenderer->BindTexture(0, m_skyBoxTexture);
	g_theRenderer->BindShader(nullptr);
	g_theRenderer->DrawVertexBufferIndex(m_skyVertexBuffer, m_skyIndexBuffer, VertexType::Vertex_PCU, static_cast<int>(m_skyIndexes.size()));
}

void Game::RenderGameTitle() const
{
	// Get mode name string
	std::string modeName = App::GetGameModeNameByString(g_theApp->m_currentGameModeType);

	// Get screen and text size
	ImVec2 screenSize = ImGui::GetIO().DisplaySize;
	ImVec2 textSize = ImGui::CalcTextSize(modeName.c_str());

	float textScale = 2.5f; // Scale for large title
	ImVec2 scaledTextSize = ImVec2(textSize.x * textScale, textSize.y * textScale);

	ImVec2 textPos = ImVec2((screenSize.x - scaledTextSize.x) * 0.5f, 10.0f);
	ImVec2 boxPadding = ImVec2(20, 10);
	ImVec2 gameModeTitleBoxMin = ImVec2(textPos.x - boxPadding.x, textPos.y - boxPadding.y);
	ImVec2 gameModeTitleBoxMax = ImVec2(textPos.x + scaledTextSize.x + boxPadding.x, textPos.y + scaledTextSize.y + boxPadding.y);

	// Access draw list before creating a window
	ImDrawList* drawList = ImGui::GetBackgroundDrawList();

	// Draw border
	drawList->AddRect(gameModeTitleBoxMin, gameModeTitleBoxMax, IM_COL32(200, 200, 200, 180), 8.0f, 0, 2.0f);

	// Create invisible overlay window
	ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
	ImGui::SetNextWindowSize(screenSize);

	ImGui::Begin("GameModeTitle", nullptr,
		ImGuiWindowFlags_NoBackground |
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoInputs |
		ImGuiWindowFlags_NoSavedSettings);

	ImGui::SetCursorPos(textPos);
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.85f, 0.3f, 1.0f)); // Warm gold
	ImGui::SetWindowFontScale(textScale);
	ImGui::Text("%s", modeName.c_str());
	ImGui::SetWindowFontScale(1.f);
	ImGui::PopStyleColor();

	ImGui::End();
}

void Game::RenderFpsAndPlayerPos()
{
	ImVec2 padding = ImVec2(10, 10);
	ImVec2 anchorPos = ImVec2(padding.x, padding.y);

	ImGui::SetNextWindowPos(anchorPos, ImGuiCond_Always);
	ImGui::SetNextWindowBgAlpha(0.7f);

	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.1f, 0.1f, 0.1f, 0.85f));
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.f, 1.f, 1.f, 0.5f));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 8.f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.f);

	ImGui::Begin("Game Info", nullptr,
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_AlwaysAutoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoSavedSettings);

	ImGui::SetWindowFontScale(2.f);

	// FPS
	ImVec4 fpsColor = m_displayedFps >= 60.f ? ImVec4(0.2f, 1.f, 0.2f, 1.f) : ImVec4(1.f, 0.2f, 0.2f, 1.f);
	ImGui::TextColored(fpsColor, "FPS:");
	ImGui::SameLine();
	ImGui::Text("%.f", m_displayedFps);

	if (ImGui::CollapsingHeader("FPS Graph", ImGuiTreeNodeFlags_DefaultOpen))
	{
		if (!m_displayedFpsHistory.empty())
		{
			float rawMaxFps = *std::max_element(m_displayedFpsHistory.begin(), m_displayedFpsHistory.end());
			float smoothingRate = 0.05f; // Smaller = smoother
			m_smoothedMaxFps = Interpolate(m_smoothedMaxFps, rawMaxFps, smoothingRate);

			float minFps = 0.f;
			float maxFps = GetClamped(m_smoothedMaxFps, 90.f, 300.f);

			ImDrawList* drawList = ImGui::GetWindowDrawList();
			ImVec2 canvasSize = ImVec2(300, 120);
			ImVec2 canvasPos = ImGui::GetCursorScreenPos();
			ImVec2 canvasEnd = canvasPos + canvasSize;

			drawList->AddRectFilled(canvasPos, canvasEnd, IM_COL32(25, 25, 25, 200), 4.f);
			drawList->AddRect(canvasPos, canvasEnd, IM_COL32(255, 255, 255, 80), 2.f);

			float spacing = canvasSize.x / (float)(m_displayedFpsHistory.size() - 1);
			for (size_t i = 1; i < m_displayedFpsHistory.size(); ++i)
			{
				float fps0 = m_displayedFpsHistory[i - 1];
				float fps1 = m_displayedFpsHistory[i];

				float norm0 = (fps0 - minFps) / (maxFps - minFps);
				float norm1 = (fps1 - minFps) / (maxFps - minFps);

				ImVec2 p0 = ImVec2(canvasPos.x + (i - 1) * spacing, canvasPos.y + canvasSize.y * (1.f - norm0));
				ImVec2 p1 = ImVec2(canvasPos.x + i * spacing, canvasPos.y + canvasSize.y * (1.f - norm1));

				ImU32 color = fps1 >= 60.f ? IM_COL32(50, 255, 50, 255) : IM_COL32(255, 50, 50, 255);
				drawList->AddLine(p0, p1, color, 2.5f);
			}

			float yYellowThreshold = canvasPos.y + canvasSize.y * (1.f - (60.f - minFps) / (maxFps - minFps));
			ImU32 yellow = IM_COL32(255, 255, 0, 180);
			drawList->AddLine(ImVec2(canvasPos.x, yYellowThreshold), ImVec2(canvasEnd.x, yYellowThreshold), yellow, 1.2f);
			drawList->AddText(ImVec2(canvasEnd.x + 4, yYellowThreshold - ImGui::GetTextLineHeight() * 0.5f), yellow, "60");

			float yRedThreshold = canvasPos.y + canvasSize.y * (1.f - (30.f - minFps) / (maxFps - minFps));
			ImU32 red = IM_COL32(255, 0, 0, 180);
			drawList->AddLine(ImVec2(canvasPos.x, yRedThreshold), ImVec2(canvasEnd.x, yRedThreshold), red, 1.2f);
			drawList->AddText(ImVec2(canvasEnd.x + 4, yRedThreshold - ImGui::GetTextLineHeight() * 0.5f), red, "30");

			ImGui::Dummy(canvasSize); // Reserve space

			// Draw Y-axis labels on the right
			std::vector<int> yTicks;
			int step = 30; // spacing between ticks
			int maxTick = static_cast<int>(maxFps / step) * step;

			for (int i = 0; i <= maxTick; i += step)
			{
				yTicks.emplace_back(i);
			}
			for (int yValue : yTicks)
			{
				float normalized = (yValue - minFps) / (maxFps - minFps);
				float yPos = canvasPos.y + canvasSize.y * (1.0f - normalized);
				if (yPos >= canvasPos.y && yPos <= canvasEnd.y)
				{
					ImVec2 lineStart = ImVec2(canvasPos.x, yPos);
					ImVec2 lineEnd = ImVec2(canvasEnd.x, yPos);
					drawList->AddLine(lineStart, lineEnd, IM_COL32(80, 80, 80, 100), 1.0f);
				}
			}

			float fpsY = canvasPos.y + canvasSize.y * (1.f - (m_displayedFps - minFps) / (m_smoothedMaxFps - minFps));
			drawList->AddLine(ImVec2(canvasPos.x, fpsY), ImVec2(canvasEnd.x, fpsY), IM_COL32(100, 200, 255, 255), 1.f);
			drawList->AddText(ImVec2(canvasEnd.x + 4, fpsY - ImGui::GetTextLineHeight() * 0.5f), IM_COL32(100, 200, 255, 255), std::to_string((int)m_displayedFps).c_str());
		}
		else
		{
			ImGui::Text("FPS history unavailable.");
		}
	}

	// Player Position
	if (m_player)
	{
		ImGui::TextColored(ImVec4(0.f, 1.f, 1.f, 1.f), "Player Pos:");
		ImGui::SameLine();
		ImGui::Text("(%.2f, %.2f, %.2f)", m_player->m_position.x, m_player->m_position.y, m_player->m_position.z);
	}
	else
	{
		ImGui::TextColored(ImVec4(1.f, 0.2f, 0.2f, 1.f), "Player Pos: N/A");
	}

	ImGui::End();

	ImGui::PopStyleColor(2);
	ImGui::PopStyleVar(2);
}

void Game::RenderGameUI()
{
 	RenderGameTitle();
	RenderFpsAndPlayerPos();
}

void Game::RenderDebugKeyWindow()
{
	ImVec2 padding = ImVec2(10, 10);
	ImVec2 screenSize = ImGui::GetIO().DisplaySize;

	// Position under the FPS panel (adjust as needed)
	ImVec2 anchorPos = ImVec2(padding.x, 270.f); // 110 to leave space below top panel
	ImGui::SetNextWindowPos(anchorPos, ImGuiCond_Always);
	ImGui::SetNextWindowBgAlpha(1.f); // We'll set color ourselves

	// Push custom style colors
	ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.2f, 0.4f, 0.6f, 1.f));       // Title bar
	ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.2f, 0.4f, 0.6f, 1.f)); // When active
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.1f, 0.1f, 0.1f, 0.85f));     // Background
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.f, 1.f, 1.f, 0.5f));        // Border

	// Push border thickness
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 8.f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.f);

	ImGui::Begin("Debug Keys", nullptr,
		ImGuiWindowFlags_AlwaysAutoResize |
		ImGuiWindowFlags_NoSavedSettings);

	ImGui::SetWindowFontScale(2.f);

	// --- Content ---
	ImGui::Text("Keybinds:");

	// Example keybinds (customize per game mode if needed)
	ImGui::Text("P - Pause/Unpause: %s", m_clock->IsPaused() ? u8"Paused" : u8"Unpaused");
	ImGui::BulletText("Arrow Left/Right - Switch game modes");
	ImGui::BulletText("O - Step One Frame");
	ImGui::BulletText("R - Restart current game mode");
	ImGui::BulletText("F11 - Show of Mouse Cursor", m_player->m_debugMouseEnabled ? u8"Enabled" : u8"Disabled");

	if (ImGui::CollapsingHeader("Game Mode Keys", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0.2f, 1.0f, 0.2f, 1.0f)); // Green 
		ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.15f, 0.15f, 1.0f)); // Default
		ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));  // Hover
		ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.2f, 0.8f, 0.2f, 1.0f));  // Active

		if (m_gameModeConfig.m_useAStar)
		{
			ImGui::Checkbox("NavMesh (F1)", &m_enableNavMeshVisual);
			ImGui::Checkbox("Agent Path (F2)", &m_enablePathVisual);
			
			if (m_gameModeConfig.m_useORCA)
			{
				ImGui::Checkbox("Heatmap (F3)", &m_enableHeatmapVisual);
				
				if (m_gameModeConfig.m_numberOfAgents <= 20)
				{
					ImGui::Checkbox("Agent FOV (F4)", &m_enableFOVZoneVisual);
				}
			}
		}
		
		if (m_gameModeConfig.m_useVO || m_gameModeConfig.m_useRVO || 
			m_gameModeConfig.m_useHRVO || m_gameModeConfig.m_useORCA)
		{
			ImGui::Checkbox("Show Current Velocity-based Algorithm (F5)", &m_enableCurrentVOAlgorithmVisual);
		}

		ImGui::PopStyleColor(4);
	}

	ImGui::End();

	// Pop styles in reverse order
	ImGui::PopStyleVar(2);     // Border size & rounding
	ImGui::PopStyleColor(4);  // TitleBg, TitleBgActive, WindowBg, Border
}

float Game::GetGameFPS() const
{
	if (m_frameTimes.empty()) return 0.f;

	float totalTime = 0.f;
	for (float dt : m_frameTimes)
	{
		totalTime += dt;
	}

	float instantFPS = 1.f / (totalTime / m_frameTimes.size());

	static float smoothedFPS = instantFPS;
	smoothedFPS = smoothedFPS * 0.9f + instantFPS * 0.1f;

	return smoothedFPS;
}
