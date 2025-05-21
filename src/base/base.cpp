#include "base.h"



#include <thread>



#include <minhook/minhook.h>



#include "main.h"

#include "java/java.h"

#include "java/hotspot/hotspot.h"

#include "java/javahook.h"

#include "util/logger.h"

#include "util/minecraft/minecraft.h"

#include "util/math/math.h"

#include "util/window/borderless.h"

#include "util/keys.h"

#include "util/render/render.h"

#include "util/string.h"

#include "menu/menu.h"

#include "moduleManager/moduleManager.h"

#include "moduleManager/commonData.h"

#include "sdk/sdk.h"

#include "sdk/net/minecraft/client/ClientBrandRetriever.h"

#include "configManager/configManager.h"

#include "notificationManager/notificationManager.h"



void Base::Init()

{

#ifdef _DEBUG

	Logger::Init();

#endif



	MH_Initialize();



	Java::Init();

	switch (Java::version)

	{

	case MinecraftVersion::LUNAR_1_8_9:

		LOG_INFO("Lunar Client 1.8.9 detected");

		break;

	case MinecraftVersion::LUNAR_1_7_10:

		LOG_INFO("Lunar Client 1.7.10 detected");

		break;

	case MinecraftVersion::VANILLA_1_8_9:

		LOG_INFO("Vanilla 1.8.9 detected");

		break;

	case MinecraftVersion::VANILLA_1_7_10:

		LOG_INFO("Vanilla 1.7.10 detected");

		break;

	case MinecraftVersion::FORGE_1_8_9:

		LOG_INFO("Forge 1.8.9 detected");

		break;

	case MinecraftVersion::FORGE_1_7_10:

		LOG_INFO("Forge 1.7.10 detected");

		break;

	case MinecraftVersion::UNKNOWN:

		LOG_INFO("Unknown Minecraft version detected");

		MessageBoxA(nullptr, "Unknown Minecraft version detected", "Ida V1", MB_OK | MB_ICONERROR);

		Logger::Shutdown();

		MH_Uninitialize();

		Java::Shutdown();

		FreeLibraryAndExitThread(Main::hModule, 0);

		break;

	default:

		LOG_INFO("Unknown Minecraft version detected");

		MessageBoxA(nullptr, "Unknown Minecraft version detected", "Ida V1", MB_OK | MB_ICONERROR);

		Logger::Shutdown();

		MH_Uninitialize();

		Java::Shutdown();

		FreeLibraryAndExitThread(Main::hModule, 0);

		break;

	}



	SDK::Init();



	if (!HotSpot::Init())

	{

		LOG_INFO("Failed to initialize HotSpot");

		MessageBoxA(nullptr, "Failed to initialize HotSpot", "Ida v1", MB_OK | MB_ICONERROR);

		Logger::Shutdown();

		MH_Uninitialize();

		Java::Shutdown();

		FreeLibraryAndExitThread(Main::hModule, 0);

		return;

	}


	Menu::Init();

	ModuleManager::Init();

	MinecraftItems::Init();



	Base::m_running = true;



	while (Base::m_running)

	{

		if (Keys::IsKeyPressed(VK_END))

		{

			Base::m_running = false;

		}



		ModuleManager::UpdateModules();

		std::this_thread::sleep_for(std::chrono::milliseconds(5));

	}



	Main::Shutdown();

}



void Base::Shutdown()

{

	if (Borderless::enabled) Borderless::Restore(Menu::handleWindow);

	JavaHook::Shutdown();

	SDK::Shutdown();

	Java::Shutdown();

	Menu::Shutdown();

	Logger::Shutdown();



	MH_Uninitialize();

}



namespace WatermarkColors { // Lokális névtér a félreértések elkerülésére

	const ImVec4 ACCENT = ImVec4(0.34f, 0.54f, 0.86f, 1.0f);

	const ImU32 ACCENT_U32 = IM_COL32(87, 138, 219, 255); // Ugyanaz, mint Colors::accent

	const ImU32 TEXT_DEFAULT_U32 = IM_COL32(255, 255, 255, 255); // Fehér

	const ImU32 SEPARATOR_LINE_U32 = IM_COL32(38, 38, 46, 200); // Colors::bg_light enyhébb alfával

}





void Base::renderLoop() // Runs every frame

{

	if (!Base::m_running || settings::Hud_DisableAllRendering) return;



	ModuleManager::RenderOverlay();



	if (Menu::openHudEditor)

	{

		ImVec2 displaySize = ImGui::GetIO().DisplaySize;

		ImGui::GetBackgroundDrawList()->AddRectFilled(ImVec2(0, 0), displaySize, IM_COL32(0, 0, 0, 100));

	}



	if (settings::Hud_Watermark)

	{

		std::time_t now = std::time(nullptr);

		std::tm localTime{};

		localtime_s(&localTime, &now); // Windows specifikus, más platformon localtime_r vagy sima localtime

		char time_buffer[9];

		std::strftime(time_buffer, sizeof(time_buffer), "%H:%M:%S", &localTime);



		const char* watermark_text_val = "IDA";

		// Használjuk a lokálisan definiált színeket

		ImU32 haze_text_color_u32 = WatermarkColors::ACCENT_U32;

		ImU32 default_stat_text_color_u32 = WatermarkColors::TEXT_DEFAULT_U32;

		ImU32 separator_line_color_u32 = WatermarkColors::SEPARATOR_LINE_U32;





		std::string version_str = Base::m_version;

		std::string fps_str = std::to_string(CommonData::fps) + " FPS";

		std::string ping_str = std::to_string(CommonData::ping) + "ms";

		std::string coords_str = "X: " + StringUtils::FloatToString(CommonData::playerPos.x, 1) + " Y: " + StringUtils::FloatToString(CommonData::playerPos.y, 1) + " Z: " + StringUtils::FloatToString(CommonData::playerPos.z, 1);

		std::string direction_str = StringUtils::YawToDirection(CommonData::playerYaw);

		std::string time_val_str = time_buffer;



		int padding = 10;

		float watermarkActualRenderSize = 24.0f;

		float statsActualRenderSize = 22.0f;



		ImFont* renderFont = Menu::fontBold ? Menu::fontBold : ImGui::GetFont();



		ImVec2 hazeSize = renderFont->CalcTextSizeA(watermarkActualRenderSize, FLT_MAX, 0, watermark_text_val);

		ImVec2 versionSize = renderFont->CalcTextSizeA(statsActualRenderSize, FLT_MAX, 0, version_str.c_str());

		ImVec2 fpsSize = renderFont->CalcTextSizeA(statsActualRenderSize, FLT_MAX, 0, fps_str.c_str());

		ImVec2 pingSize = renderFont->CalcTextSizeA(statsActualRenderSize, FLT_MAX, 0, ping_str.c_str());

		ImVec2 coordsSize = renderFont->CalcTextSizeA(statsActualRenderSize, FLT_MAX, 0, coords_str.c_str());

		ImVec2 directionSize = renderFont->CalcTextSizeA(statsActualRenderSize, FLT_MAX, 0, direction_str.c_str());

		ImVec2 timeSize = renderFont->CalcTextSizeA(statsActualRenderSize, FLT_MAX, 0, "00:00:00");





		float calculatedTotalWidth = (float)padding;

		calculatedTotalWidth += hazeSize.x;



		// A szeparátor szélessége (vonal + padding mindkét oldalon)

		// Legyen a padding fele a vonal mindkét oldalán

		float separatorSectionWidth = (float)padding;



		if (settings::Hud_WatermarkVersion) { calculatedTotalWidth += separatorSectionWidth + versionSize.x; }

		if (settings::Hud_WatermarkFps) { calculatedTotalWidth += separatorSectionWidth + fpsSize.x; }

		if (settings::Hud_WatermarkPing) { calculatedTotalWidth += separatorSectionWidth + pingSize.x; }

		if (settings::Hud_WatermarkCoords) { calculatedTotalWidth += separatorSectionWidth + coordsSize.x; }

		if (settings::Hud_WatermarkDirection) { calculatedTotalWidth += separatorSectionWidth + directionSize.x; }

		if (settings::Hud_WatermarkTime) { calculatedTotalWidth += separatorSectionWidth + timeSize.x; }

		calculatedTotalWidth += padding;



		float maxTextHeight = watermarkActualRenderSize; // A watermark magassága a mérvadó

		ImVec2 rectSize = ImVec2(calculatedTotalWidth, maxTextHeight + padding * 2);



		ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoResize;

		if (!Menu::openHudEditor) {

			windowFlags |= ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoMouseInputs;

		}



		if (Menu::setupWatermarkFlag) { // Feltételezve, hogy Menu::setupWatermarkFlag egy std::unique_ptr<std::once_flag>

			std::call_once(*Menu::setupWatermarkFlag, [&]() {

				ImGui::SetNextWindowPos(ImVec2(settings::Hud_WatermarkPosition[0], settings::Hud_WatermarkPosition[1]));

				});

		}

		else {

			ImGui::SetNextWindowPos(ImVec2(settings::Hud_WatermarkPosition[0], settings::Hud_WatermarkPosition[1]), ImGuiCond_Appearing);

		}



		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 5.0f);

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0)); // Nincs belsõ ablak-padding, mi kezeljük


		// ImGui::PushStyleColor(ImGuiCol_WindowBg, WatermarkColors::BACKGROUND_U32); // Ha szeretnél egyedi hátteret



		ImGui::SetNextWindowSize(rectSize);

		ImGui::Begin("Watermark", nullptr, windowFlags);

		{

			if (Menu::openHudEditor && ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) { // Csak akkor frissítjük, ha tényleg mozgatják

				settings::Hud_WatermarkPosition[0] = ImGui::GetWindowPos().x;

				settings::Hud_WatermarkPosition[1] = ImGui::GetWindowPos().y;

			}



			ImDrawList* drawList = ImGui::GetWindowDrawList();

			ImVec2 currentScreenPos = ImGui::GetWindowPos();

			currentScreenPos.x += padding;

			currentScreenPos.y += padding;



			// "HAZE" szöveg

			drawList->AddText(renderFont, watermarkActualRenderSize, currentScreenPos, haze_text_color_u32, watermark_text_val);

			currentScreenPos.x += hazeSize.x;



			// Lambda a statisztikák és elválasztók rajzolásához

			auto renderStatEntry =

				[&](const std::string& statText, const ImVec2& statSize)

				{

					currentScreenPos.x += padding / 2.0f; // Térköz a szeparátor elõtt



					// Vertikális szeparátor manuális rajzolása

					float line_x = currentScreenPos.x + (padding / 2.0f) / 2.0f; // Középre a padding/2 részen belül

					drawList->AddLine(

						ImVec2(line_x, currentScreenPos.y),

						ImVec2(line_x, currentScreenPos.y + maxTextHeight),

						separator_line_color_u32,

						1.0f

					);

					currentScreenPos.x += padding / 2.0f; // Térköz a szeparátor után (ezzel lesz a teljes separatorSectionWidth = padding)



					// Szöveg igazítása a magassághoz képest

					float y_offset = (maxTextHeight - statSize.y) / 2.0f;

					drawList->AddText(renderFont, statsActualRenderSize, ImVec2(currentScreenPos.x, currentScreenPos.y + y_offset), default_stat_text_color_u32, statText.c_str());

					currentScreenPos.x += statSize.x;

				};



			if (settings::Hud_WatermarkVersion) { renderStatEntry(version_str, versionSize); }

			if (settings::Hud_WatermarkFps) { renderStatEntry(fps_str, fpsSize); }

			if (settings::Hud_WatermarkPing) { renderStatEntry(ping_str, pingSize); }

			if (settings::Hud_WatermarkCoords) { renderStatEntry(coords_str, coordsSize); }

			if (settings::Hud_WatermarkDirection) { renderStatEntry(direction_str, directionSize); }

			if (settings::Hud_WatermarkTime) { renderStatEntry(time_val_str, timeSize); }

		}

		ImGui::End();



		// ImGui::PopStyleColor(); // Ha pusholtál WindowBg-t

		ImGui::PopStyleVar(2);

	}



	ModuleManager::RenderHud();

	NotificationManager::Render();

}