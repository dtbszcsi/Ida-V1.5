#include "hitSelect.h" 

#include <chrono>
#include <string>    
#include <vector>    
#include <cmath>     
#include <windows.h> 

#include "menu/menu.h"                 
#include "moduleManager/commonData.h"  
#include "util/minecraft/minecraft.h"  
#include "util/logger.h" // LOG_INFO, LOG_ERROR (ha használod)

// A KONSTRUKTOR DEFINÍCIÓJA INNEN TÖRÖLVE LETT, MERT A .h-BAN VAN INLINE.

// --- Segédfüggvények Implementációja ---

float HitSelect::GetDistanceToEntity(CEntity& target) {
    if (target.GetInstance() == nullptr || !SDK::minecraft || !SDK::minecraft->thePlayer || !SDK::minecraft->thePlayer->GetInstance()) {
        return FLT_MAX;
    }
    Vector3 targetPos = target.GetPos();
    Vector3 playerPos = CommonData::playerPos;
    float xDiff = targetPos.x - playerPos.x;
    float yDiff = targetPos.y - playerPos.y;
    float zDiff = targetPos.z - playerPos.z;
    return sqrtf(xDiff * xDiff + yDiff * yDiff + zDiff * zDiff);
}

bool HitSelect::IsValidAttackableTarget(CEntity& target) {
    if (target.GetInstance() == nullptr || !SDK::minecraft || !SDK::minecraft->thePlayer || !SDK::minecraft->thePlayer->GetInstance()) {
        return false;
    }
    if (target.GetInstance() == SDK::minecraft->thePlayer->GetInstance()) {
        return false;
    }
    // TODO: Bõvítsd ezt az SDK-d alapján (pl. IsDead, IsPlayer, IsFriend ellenõrzések)
    return true;
}

void HitSelect::SimulateAttack() {
    HWND game_window_handle = Menu::handleWindow;
    if (!game_window_handle || !IsWindow(game_window_handle)) {
        game_window_handle = FindWindowA("LWJGL", nullptr); // Vagy a te játékablakod azonosítója
        if (!game_window_handle || !IsWindow(game_window_handle)) {
            // LOG_ERROR("HitSelect::SimulateAttack - Valid game window handle NOT found!");
            return;
        }
        Menu::handleWindow = game_window_handle; // Mentsük el, ha megtaláltuk
    }

    POINT cursorPos; // Nem biztos, hogy a cursorPos szükséges a PostMessage-hez, ha az ablaknak küldöd
    GetCursorPos(&cursorPos);
    // ScreenToClient(game_window_handle, &cursorPos); // Fontold meg, ha szükséges

    PostMessage(game_window_handle, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(0, 0)); // Próbáld meg (0,0) koordinátákkal
    PostMessage(game_window_handle, WM_LBUTTONUP, 0, MAKELPARAM(0, 0));
}

// --- HitSelect Fõ Logikája ---
void HitSelect::Update() {
    if (!settings::HS_Enabled) return;
    if (Menu::open) return;

    if (!CommonData::SanityCheck()) return;
    if (!SDK::minecraft || !SDK::minecraft->thePlayer || !SDK::minecraft->thePlayer->GetInstance() || SDK::minecraft->IsInGuiState()) {
        return;
    }

    if (!(GetAsyncKeyState(VK_LBUTTON) & 0x8000)) return;

    if (settings::HS_WeaponOnly) {
        if (!SDK::minecraft->thePlayer->GetInventory().GetInstance()) { return; }
        CItemStack currentItem = SDK::minecraft->thePlayer->GetInventory().GetCurrentItem(); // JAVÍTVA CItemStack-re
        if (currentItem.GetInstance() == nullptr || !MinecraftUtils::IsWeapon(currentItem)) { // currentItem használata
            return;
        }
    }

    CEntity target = SDK::minecraft->GetMouseOver().GetEntityHit();

    if (!IsValidAttackableTarget(target)) {
        return;
    }

    const float fixedHitRange = 3.0f; // Fix hatótáv
    float distance = GetDistanceToEntity(target);
    if (distance > fixedHitRange || distance == FLT_MAX) {
        return;
    }

    long long currentTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();

    long long currentAttackCooldownMs = settings::HS_AttackCooldownMs; // Használjuk a beállított értéket

    if (currentTimeMs - m_lastAttackTime < currentAttackCooldownMs) {
        return;
    }

    int hurtTime = target.GetHurtResistantTime();
    if (settings::HS_Mode == 1) { // Perfect Timing
        if (hurtTime == 0 || hurtTime == 10) {
            SimulateAttack();
            m_lastAttackTime = currentTimeMs;
        }
    }
    else { // Normal Mode (settings::HS_Mode == 0)
        SimulateAttack();
        m_lastAttackTime = currentTimeMs;
    }
}

// --- Menü Renderelése (Módosítva a kérésednek megfelelõen) ---
void HitSelect::RenderMenu() {
    // A "Bind" gomb és a fõ "Enabled" kapcsoló marad az eredeti Menu::ToggleWithKeybind hívással
    Menu::ToggleWithKeybind(&settings::HS_Enabled, settings::HS_Key);

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10.f);
    ImGui::Separator();
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10.f);

    // "Weapon Only" checkbox standard ImGui::Checkbox-szal a pipához
    ImGui::Checkbox("Weapon Only##HS_WeaponOnly", &settings::HS_WeaponOnly);


    Menu::Slider("Attack Cooldown (ms)", &settings::HS_AttackCooldownMs, 50, 1200);

    if (ImGui::IsItemHovered()) { // Ezt a Menu::Slider hívása után kellene tenni
        ImGui::SetTooltip("Lower value = faster attacking\n(e.g., 625ms is ~1.6 CPS, 100ms is 10 CPS)");
    }

    const char* modes[] = { "Normal (Cooldown)", "Perfect Timing (HurtTime)" };
    if (ImGui::BeginCombo("Mode##HSModeCombo", modes[settings::HS_Mode])) {
        for (int i = 0; i < IM_ARRAYSIZE(modes); i++) {
            const bool is_selected = (settings::HS_Mode == i);
            if (ImGui::Selectable(modes[i], is_selected)) {
                settings::HS_Mode = i;
            }
            if (is_selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    // A státusz infók (cooldown, hurt time) korábban ki lettek véve.
}