#include "rightAutoClicker.h"

#include <chrono>
#include <random>
#include <algorithm> // std::max

#include "sdk/sdk.h"
#include "menu/menu.h"
#include "util/logger.h"
// #include "settings.h" // Ha szükséges

void RightAutoClicker::Update()
{
	if (!settings::RAC_Enabled) return;
	if (Menu::open) return;

	bool isInInventoryScreen = SDK::minecraft->IsInInventory();

	// --- ÚJ "ONLY INVENTORY" ELLENÕRZÉS ---
	if (settings::RAC_onlyInventory && !isInInventoryScreen) {
		// Ha az "Only Inventory" mód aktív, de NEM vagyunk az inventory-ban, akkor kilépünk.
		return;
	}
	// ------------------------------------

	// Eredeti GUI ellenõrzés, ami most már csak akkor számít, ha NEM vagyunk "Only Inventory" módban.
	// Kezeli az "Allow Inventory" logikát normál módban.
	if (!settings::RAC_onlyInventory && SDK::minecraft->IsInGuiState() && !(settings::RAC_allowInventory && isInInventoryScreen)) {
		return; // Normál mód: Ne fusson GUI-ban, kivéve ha az inventory ÉS engedélyezve van
	}

	// --- A kód többi része nagyrészt változatlan ---
	if (SDK::minecraft->thePlayer->GetInventory().GetCurrentItem().GetInstance() == NULL) return;
	if (settings::RAC_blocksOnly && SDK::minecraft->thePlayer->GetInventory().GetCurrentItem().GetItem().GetUnlocalizedName().find("tile") == std::string::npos) return;

	long milli = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	if (m_lastClickTime == 0) m_lastClickTime = milli;
	if (m_nextCps <= 0) m_nextCps = 1; // Biztonsági ellenõrzés 0 CPS ellen

	if ((milli - m_lastClickTime) < (1000 / m_nextCps)) return;

	if (GetAsyncKeyState(VK_RBUTTON) & 0x8000) {
		POINT pos_cursor;
		GetCursorPos(&pos_cursor);
		SendMessage(Menu::handleWindow, WM_RBUTTONDOWN, MK_RBUTTON, MAKELPARAM(pos_cursor.x, pos_cursor.y));
		SendMessage(Menu::handleWindow, WM_RBUTTONUP, 0, MAKELPARAM(pos_cursor.x, pos_cursor.y));

		m_lastClickTime = milli;

		// Az inventory szorzó továbbra is érvényesülhet "Only Inventory" módban is, ha kell
		float multiplier = (isInInventoryScreen && (settings::RAC_onlyInventory || settings::RAC_allowInventory)) ? settings::Rac_inventoryMultiplier : 1.0f;


		int minCpsRaw = settings::RAC_rightMinCps;
		int maxCpsRaw = settings::RAC_rightMaxCps;
		int minCps = (std::max)(1, static_cast<int>(minCpsRaw * multiplier));
		int maxCps = (std::max)(1, static_cast<int>(maxCpsRaw * multiplier));
		if (minCps > maxCps) {
			maxCps = minCps;
		}

		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_int_distribution<> distrib(minCps, maxCps);
		m_nextCps = distrib(gen);
	}
}

void RightAutoClicker::RenderMenu()
{
	// A fõ ki/be kapcsoló és bind gomb marad az eredeti Menu::ToggleWithKeybind hívással
	Menu::ToggleWithKeybind(&settings::RAC_Enabled, settings::RAC_Key);

	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10.f);
	// Menu::HorizontalSeparator("Sep1"); // Ehelyett standard ImGui::Separator
	ImGui::Separator();
	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10.f);

	// A Menu::Slider hívások maradnak, ahogy kérted
	Menu::Slider("Min CPS", &settings::RAC_rightMinCps, 1, settings::RAC_rightMaxCps); // Feltételezve, hogy ezek int-ek
	Menu::Slider("Max CPS", &settings::RAC_rightMaxCps, settings::RAC_rightMinCps, 25);

	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10.f);
	ImGui::Separator(); // Menu::HorizontalSeparator("Sep2") helyett
	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10.f);

	// Menu::Checkbox helyett ImGui::Checkbox a pipához
	ImGui::Checkbox("Only in Inventory##RAC_OnlyInv", &settings::RAC_onlyInventory);

	// Az "Allow in Inventory" letiltása, ha az "Only Inventory" be van kapcsolva
	// A BeginDisabled/EndDisabled blokk megmarad
	if (settings::RAC_onlyInventory) {
		ImGui::BeginDisabled(true);
	}

	ImGui::Checkbox("Allow in Inventory##RAC_AllowInv", &settings::RAC_allowInventory);
	if (settings::RAC_allowInventory && !settings::RAC_onlyInventory) // Csak akkor aktív, ha nincs letiltva
	{
		// A Menu::Slider hívás marad
		Menu::Slider("Inventory Multiplier", &settings::Rac_inventoryMultiplier, 1.f, 10.f);
	}

	if (settings::RAC_onlyInventory) {
		ImGui::EndDisabled();
	}
	// -------------------------------

	ImGui::Checkbox("Blocks Only##RAC_BlocksOnly", &settings::RAC_blocksOnly);
}