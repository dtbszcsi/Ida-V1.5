#include "menu.h" // Ennek tartalmaznia kell a Menu n�vt�r/oszt�ly �s a FontSize enum deklar�ci�j�t
#include <imgui/imgui_internal.h> // �vatosan haszn�ld
#include "base/base.h" // Base::m_version, Base::m_running
#include "moduleManager/moduleManager.h"
#include "configManager/configManager.h"
#include "notificationManager/notificationManager.h"
#include "configManager/settings.h" // settings:: v�ltoz�k
#include "util/logger.h" // LOG_WARN, LOG_ERROR haszn�lat�hoz (ha van ilyen loggered)
#include "LoginUI.h" // <<<--- A LOGIN UI HEADER FÁJL VISSZAADVA
#include <string>
#include <vector>
#include <map>
#include <memory> // std::unique_ptr, std::make_unique
#include <mutex>  // std::once_flag
#include <shellapi.h> // ShellExecuteA
#include <algorithm> // std::min, std::max
#include <cmath>      // std::ceil, std::floor

// FONTOS: Gy�z�dj meg r�la, hogy a FontSize enum (�s a Menu n�vt�r/oszt�ly relev�ns r�szei, pl. fontok)
// helyesen vannak defini�lva �s el�rhet�ek ebben a f�jlban (pl. a menu.h-n kereszt�l).

// Konstansok (az �ltalad prefer�lt, Menu:: n�lk�li FontSize haszn�lattal)
const FontSize CATEGORY_FONT_SIZE = FontSize::SIZE_20;
const FontSize MODULE_FONT_SIZE = FontSize::SIZE_16;
const FontSize BUTTON_FONT_SIZE = FontSize::SIZE_16;
const float ANIMATION_SPEED = 0.08f;
const float HOVER_ALPHA = 0.2f;

namespace Colors {
    // Az �ltalad defini�lt sz�nek maradnak
    const ImVec4 accent = ImVec4(0.34f, 0.54f, 0.86f, 1.0f);
    const ImVec4 bg_dark = ImVec4(0.11f, 0.11f, 0.13f, 1.0f);
    const ImVec4 bg_light = ImVec4(0.15f, 0.15f, 0.18f, 1.0f);
    const ImVec4 text = ImVec4(0.86f, 0.86f, 0.86f, 1.0f);
    const ImVec4 text_dim = ImVec4(0.60f, 0.60f, 0.60f, 1.0f);
    const ImVec4 red_button = ImVec4(0.8f, 0.2f, 0.2f, 1.0f);
    const ImVec4 green_button = ImVec4(0.2f, 0.7f, 0.3f, 1.0f);
}

namespace ModernUI {
    void DrawPanel(ImVec2 pos, ImVec2 size, ImColor color, float rounding = 5.0f, int flags = ImDrawFlags_RoundCornersAll) {
        ImGui::GetWindowDrawList()->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), color, rounding, flags);
    }

    bool AnimatedButton(const char* label, ImVec2 size, FontSize fontSize = FontSize::SIZE_16, bool selected = false, const ImVec4* pOptionalBaseColor = nullptr, const ImVec4* pOptionalTextColor = nullptr) {
        static std::map<ImGuiID, float> hoverAnimations;
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems) return false;

        ImGuiID id = window->GetID(label);
        auto it = hoverAnimations.find(id);
        if (it == hoverAnimations.end()) {
            it = hoverAnimations.insert({ id, 0.0f }).first;
        }

        ImVec2 p_min = ImGui::GetCursorScreenPos();
        ImVec2 p_max = ImVec2(p_min.x + size.x, p_min.y + size.y);
        bool hovered = ImGui::IsMouseHoveringRect(p_min, p_max) && ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);

        float& button_specific_hover_alpha_effect = it->second;
        if (hovered) button_specific_hover_alpha_effect = ImMin(button_specific_hover_alpha_effect + ANIMATION_SPEED, HOVER_ALPHA);
        else button_specific_hover_alpha_effect = ImMax(button_specific_hover_alpha_effect - ANIMATION_SPEED, 0.0f);

        ImColor finalBgColor;
        ImVec4 textColorToUse = pOptionalTextColor ? *pOptionalTextColor : Colors::text;

        if (selected) {
            finalBgColor = ImColor(Colors::accent.x, Colors::accent.y, Colors::accent.z, 0.3f + button_specific_hover_alpha_effect);
        }
        else {
            if (pOptionalBaseColor) {
                float baseAlpha = pOptionalBaseColor->w;
                float currentAlpha = hovered ? ImMin(baseAlpha + 0.15f, 1.0f) : baseAlpha;
                finalBgColor = ImColor(pOptionalBaseColor->x, pOptionalBaseColor->y, pOptionalBaseColor->z, currentAlpha);
            }
            else {
                finalBgColor = ImColor(Colors::bg_light.x, Colors::bg_light.y, Colors::bg_light.z, button_specific_hover_alpha_effect);
            }
        }

        ImGui::GetWindowDrawList()->AddRectFilled(p_min, p_max, finalBgColor, 4.0f);

        if (selected && !pOptionalBaseColor) {
            ImGui::GetWindowDrawList()->AddRectFilled(p_min, ImVec2(p_min.x + 3.0f, p_max.y), ImColor(Colors::accent), 2.0f, ImDrawFlags_RoundCornersLeft);
        }

        ImFont* requestedFont = nullptr;
        switch (fontSize) {
        case FontSize::SIZE_16: requestedFont = Menu::font16; break;
        case FontSize::SIZE_18: requestedFont = Menu::font18; break;
        case FontSize::SIZE_20: requestedFont = Menu::font20; break;
        case FontSize::SIZE_24: requestedFont = Menu::font24; break;
        default: requestedFont = ImGui::GetFont(); break;
        }

        ImFont* targetFont = ImGui::GetFont();
        if (requestedFont) {
            if (requestedFont->IsLoaded()) {
                targetFont = requestedFont;
            }
            else {
                // LOG_WARN("AnimatedButton: Requested font for label '%s' (size enum %d) is not loaded. Falling back.", label, static_cast<int>(fontSize));
            }
        }

        if (targetFont && targetFont->IsLoaded()) {
            float currentFontSize = targetFont->FontSize;
            ImVec2 textSize = targetFont->CalcTextSizeA(currentFontSize, FLT_MAX, 0.0f, label);
            float textPosX = p_min.x + (size.x - textSize.x) * 0.5f;
            float textPosY = p_min.y + (size.y - textSize.y) * 0.5f;

            ImGui::PushFont(targetFont);
            ImGui::GetWindowDrawList()->AddText(ImVec2(textPosX, textPosY), ImColor(textColorToUse), label);
            ImGui::PopFont();
        }
        else {
            const char* errorText = "FONT_ERR";
            ImVec2 errorTextSize = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFont()->FontSize, FLT_MAX, 0.0f, errorText);
            ImGui::GetWindowDrawList()->AddText(
                ImVec2(p_min.x + (size.x - errorTextSize.x) * 0.5f, p_min.y + (size.y - errorTextSize.y) * 0.5f),
                IM_COL32(255, 0, 0, 255),
                errorText
            );
        }

        ImGui::SetCursorScreenPos(p_min);
        return ImGui::InvisibleButton(label, size);
    }
}

std::unique_ptr<std::once_flag> setConfigName_flag;

void Menu::RenderMenu() {
    // === LOGIN ÁLLAPOT ÉS PUFFEREK VISSZAÁLLÍTVA ===
    static bool g_isAuthenticated = false; // Kezdetben nincs bejelentkezve
    static char g_usernameBuffer[128] = "";
    static char g_passwordBuffer[128] = "";
    // === LOGIN VÉGE ===

    static int selectedCategory = 0;
    static std::vector<std::string> categories = ModuleManager::GetCategories();
    static int selectedModule = 0;
    static std::vector<std::unique_ptr<ModuleBase>>& modules = ModuleManager::GetModules();

    if (!setConfigName_flag) {
        setConfigName_flag = std::make_unique<std::once_flag>();
    }

    static float menuAlpha = 0.0f;
    if (Menu::open) {
        menuAlpha = ImMin(menuAlpha + ANIMATION_SPEED, 1.0f);
    }
    else {
        menuAlpha = ImMax(menuAlpha - ANIMATION_SPEED, 0.0f);
        if (menuAlpha == 0.0f) return;
    }

    // === LOGIN ABLAK MEGJELENÍTÉSE (ha szükséges) VISSZAÁLLÍTVA ===
    if (!g_isAuthenticated)
    {
        // Alkalmazzuk a fade effektet a login ablakra is
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, menuAlpha);
        // Meghívjuk a login ablak renderelőjét
        // A LoginUI::RenderLoginWindow feltételezetten a LoginUI.cpp-ben van definiálva
        // és frissíti a g_isAuthenticated, g_usernameBuffer, g_passwordBuffer értékeket.
        LoginUI::RenderLoginWindow(g_isAuthenticated, g_usernameBuffer, sizeof(g_usernameBuffer), g_passwordBuffer, sizeof(g_passwordBuffer));
        ImGui::PopStyleVar(); // Alpha stílus visszaállítása
        // Mivel a login ablak látható, a főmenüt még nem rajzoljuk ki erre a frame-re.
        return;
    }
    // === LOGIN ELLENŐRZÉS VÉGE ===

    // === FŐMENÜ RENDERELÉSE (csak ha be van jelentkezve) ===
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4 originalChildBg = style.Colors[ImGuiCol_ChildBg];

    // === STÍLUS BEÁLLÍTÁSOK KEZDETE (MÓDOSÍTOTT ÉRTÉKEKKEL A JOBB LÁTHATÓSÁGÉRT) ===
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(Colors::bg_dark.x, Colors::bg_dark.y, Colors::bg_dark.z, menuAlpha * Colors::bg_dark.w));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(Colors::bg_light.x, Colors::bg_light.y, Colors::bg_light.z, menuAlpha * 0.85f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(Colors::bg_dark.x, Colors::bg_dark.y, Colors::bg_dark.z, menuAlpha * 0.7f));
    ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(Colors::bg_dark.x, Colors::bg_dark.y, Colors::bg_dark.z, menuAlpha * 0.6f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(Colors::text.x, Colors::text.y, Colors::text.z, menuAlpha * Colors::text.w));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(Colors::accent.x, Colors::accent.y, Colors::accent.z, menuAlpha * 0.7f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(Colors::accent.x, Colors::accent.y, Colors::accent.z, menuAlpha * 0.85f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(Colors::accent.x, Colors::accent.y, Colors::accent.z, menuAlpha * 1.0f));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(Colors::bg_dark.x, Colors::bg_dark.y, Colors::bg_dark.z, menuAlpha * 0.75f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(Colors::bg_dark.x, Colors::bg_dark.y, Colors::bg_dark.z, menuAlpha * 0.9f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(Colors::bg_dark.x, Colors::bg_dark.y, Colors::bg_dark.z, menuAlpha * 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(Colors::bg_dark.x, Colors::bg_dark.y, Colors::bg_dark.z, menuAlpha * 0.7f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(Colors::bg_dark.x, Colors::bg_dark.y, Colors::bg_dark.z, menuAlpha * 0.85f));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(Colors::accent.x, Colors::accent.y, Colors::accent.z, menuAlpha * 0.55f));
    ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(Colors::accent.x, Colors::accent.y, Colors::accent.z, menuAlpha * 0.85f));
    ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(Colors::accent.x, Colors::accent.y, Colors::accent.z, menuAlpha * 1.0f));
    ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(Colors::accent.x, Colors::accent.y, Colors::accent.z, menuAlpha * 1.0f));
    // === STÍLUS BEÁLLÍTÁSOK VÉGE ===

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 6.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_GrabRounding, 4.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(15.0f, 15.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 6.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10.0f, 8.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 10.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarRounding, 6.0f);

    ImGui::SetNextWindowSize(ImVec2(900.0f, 580.0f), ImGuiCond_Once);
    ImGui::SetNextWindowBgAlpha(menuAlpha);

    if (ImGui::Begin("Ida", &Menu::open, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
    {
        float headerHeight = 60.0f;
        ImVec2 windowPos = ImGui::GetWindowPos();
        ImVec2 headerDrawStart = ImVec2(windowPos.x + style.WindowPadding.x, windowPos.y + style.WindowPadding.y);
        float headerContentWidth = ImGui::GetWindowContentRegionMax().x;

        ImGui::GetWindowDrawList()->AddRectFilled(
            headerDrawStart,
            ImVec2(headerDrawStart.x + headerContentWidth, headerDrawStart.y + headerHeight),
            ImColor(Colors::bg_light.x, Colors::bg_light.y, Colors::bg_light.z, menuAlpha),
            8.0f, ImDrawFlags_RoundCornersTop);

        ImFont* boldFont = Menu::fontBold ? Menu::fontBold : ImGui::GetFont();
        ImFont* regularFont = Menu::font16 ? Menu::font16 : ImGui::GetFont();
        float logoHeight = boldFont->FontSize;
        float versionHeight = regularFont->FontSize;

        ImGui::SetCursorPos(ImVec2(style.WindowPadding.x + 20.0f,
            style.WindowPadding.y + (headerHeight - logoHeight) / 2.0f - (versionHeight / 4.0f)));
        ImGui::PushFont(boldFont);
        ImGui::TextColored(ImColor(Colors::accent), "IDA");
        ImGui::PopFont();

        ImGui::SameLine(0, 5.0f);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (logoHeight - versionHeight) / 2.0f + 1.0f);
        ImGui::PushFont(regularFont);
        ImGui::TextColored(ImColor(Colors::text_dim), Base::m_version);
        ImGui::PopFont();

        float tabStartX = 160.0f;
        float availableWidthForTabs = headerContentWidth - tabStartX - style.WindowPadding.x;
        int numTotalTabs = categories.empty() ? 2 : static_cast<int>(categories.size()) + 2;
        float tabWidth = (numTotalTabs > 0 && availableWidthForTabs > 0) ? (availableWidthForTabs / numTotalTabs) : 80.0f;
        tabWidth = ImMax(tabWidth, 70.0f);
        float currentTabRelativeX = tabStartX;

        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
        for (size_t i = 0; i < categories.size(); i++) {
            ImGui::SetCursorPos(ImVec2(currentTabRelativeX, style.WindowPadding.y + (headerHeight - 30.0f) / 2.0f));
            bool isSelected = (selectedCategory == static_cast<int>(i));
            if (ModernUI::AnimatedButton(categories[i].c_str(), ImVec2(tabWidth - 5.0f, 30.0f), CATEGORY_FONT_SIZE, isSelected)) {
                selectedCategory = static_cast<int>(i);
                selectedModule = ModuleManager::GetFirstModuleIndexByCategory(categories[i]);
            }
            currentTabRelativeX += tabWidth;
        }

        ImGui::SetCursorPos(ImVec2(currentTabRelativeX, style.WindowPadding.y + (headerHeight - 30.0f) / 2.0f));
        if (ModernUI::AnimatedButton("Configs", ImVec2(tabWidth - 5.0f, 30.0f), CATEGORY_FONT_SIZE, selectedCategory == -1)) {
            selectedCategory = -1;
            selectedModule = 0;
        }
        currentTabRelativeX += tabWidth;

        ImGui::SetCursorPos(ImVec2(currentTabRelativeX, style.WindowPadding.y + (headerHeight - 30.0f) / 2.0f));
        if (ModernUI::AnimatedButton("Settings", ImVec2(tabWidth - 5.0f, 30.0f), CATEGORY_FONT_SIZE, selectedCategory == -2)) {
            selectedCategory = -2;
            selectedModule = 0;
        }
        ImGui::PopStyleVar(); // FrameRounding a tabokhoz

        ImGui::SetCursorPos(ImVec2(style.WindowPadding.x, style.WindowPadding.y + headerHeight + style.ItemSpacing.y));
        float leftPanelWidth = 200.0f;
        float availableFullHeight = ImGui::GetContentRegionAvail().y;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 8.0f)); // LeftPanel bels� padding
        ImGui::BeginChild("##LeftPanel_Wrapper", ImVec2(leftPanelWidth, availableFullHeight), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        {
            float detachButtonHeightWithSpacing = 30.0f + style.ItemSpacing.y;
            // Ha van "Logout" gomb, annak magasságát is hozzá kell adni, ha látható
            // Például: if (g_isAuthenticated) detachButtonHeightWithSpacing += (30.0f + style.ItemSpacing.y);

            ImGui::BeginChild("##ModulesList_ScrollablePart", ImVec2(0, -detachButtonHeightWithSpacing), false);
            {
                if (selectedCategory >= 0 && selectedCategory < static_cast<int>(categories.size())) {
                    for (size_t i = 0; i < modules.size(); i++) {
                        if (!modules[i] || modules[i]->GetCategory() != categories[selectedCategory]) continue;
                        bool isSelected = (selectedModule == static_cast<int>(i));
                        if (ModernUI::AnimatedButton(modules[i]->GetName().c_str(), ImVec2(ImGui::GetContentRegionAvail().x, 36.0f), MODULE_FONT_SIZE, isSelected)) {
                            selectedModule = static_cast<int>(i);
                        }
                        ImGui::Spacing();
                    }
                }
                else if (selectedCategory == -1) { // Configs f�l bal oldali list�ja
                    static std::vector<std::string> configTabs = { "Local Configs" };
                    for (size_t i = 0; i < configTabs.size(); i++) {
                        bool isSelected = (selectedModule == static_cast<int>(i));
                        if (ModernUI::AnimatedButton(configTabs[i].c_str(), ImVec2(ImGui::GetContentRegionAvail().x, 36.0f), MODULE_FONT_SIZE, isSelected)) {
                            selectedModule = static_cast<int>(i);
                        }
                        ImGui::Spacing();
                    }
                }
                else if (selectedCategory == -2) { // Settings f�l bal oldali list�ja
                    static std::vector<std::string> settingTabs = { "General", "Friends", "HUD" };
                    for (size_t i = 0; i < settingTabs.size(); i++) {
                        bool isSelected = (selectedModule == static_cast<int>(i));
                        if (ModernUI::AnimatedButton(settingTabs[i].c_str(), ImVec2(ImGui::GetContentRegionAvail().x, 36.0f), MODULE_FONT_SIZE, isSelected)) {
                            selectedModule = static_cast<int>(i);
                        }
                        ImGui::Spacing();
                    }
                }
            }
            ImGui::EndChild(); // ModulesList_ScrollablePart

            ImVec4 detachButtonBaseColor = ImVec4(Colors::red_button.x, Colors::red_button.y, Colors::red_button.z, 0.6f);
            ImVec4 detachButtonTextColor = Colors::text;
            if (ModernUI::AnimatedButton("Detach", ImVec2(ImGui::GetContentRegionAvail().x, 30.0f), BUTTON_FONT_SIZE, false, &detachButtonBaseColor, &detachButtonTextColor)) {
                Base::m_running = false;
            }

            // === LOGOUT GOMB (OPCIONÁLIS) VISSZAÁLLÍTVA ===
            ImGui::Spacing(); // Kis hely a gombok között
            ImVec4 logoutButtonBaseColor = ImVec4(0.5f, 0.5f, 0.5f, 0.6f); // Szürke gomb
            if (ModernUI::AnimatedButton("Logout", ImVec2(ImGui::GetContentRegionAvail().x, 30.0f), BUTTON_FONT_SIZE, false, &logoutButtonBaseColor, &detachButtonTextColor)) {
                g_isAuthenticated = false;
                g_usernameBuffer[0] = '\0'; // Felhasználónév törlése
                g_passwordBuffer[0] = '\0'; // Jelszó törlése
                // A LoginUI.cpp-ben lévő `loginMessage` törlése nem közvetlenül innen történik,
                // de a következő bejelentkezési kísérletnél a LoginUI újraértékeli.
                // A showPassword-t is a LoginUI kezeli.
                // Esetleg visszaállíthatod a kategóriát/modult egy alapértelmezettre
                if (!categories.empty()) {
                    selectedCategory = 0;
                    selectedModule = ModuleManager::GetFirstModuleIndexByCategory(categories[0]);
                }
                else {
                    selectedCategory = -1; // Vagy Configs/Settings, ha nincsenek modul kategóriák
                    selectedModule = 0;
                }
            }
            // === LOGOUT GOMB VÉGE ===
        }
        ImGui::EndChild(); // LeftPanel_Wrapper
        ImGui::PopStyleVar(); // LeftPanel bels� padding

        ImGui::SameLine(0.0f, style.ItemSpacing.x);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(15.0f, 15.0f)); // RightPanel bels� padding
        ImGui::BeginChild("##RightPanel_ContentArea", ImVec2(0, availableFullHeight), true);
        {
            // === JOBB OLDALI PANEL TARTALMÁNAK RENDERELÉSE ===
            if (selectedCategory >= 0 && selectedCategory < static_cast<int>(categories.size())) { // Modulok f�le
                bool moduleValid = false;
                if (selectedModule >= 0 && selectedModule < static_cast<int>(modules.size()) && modules[selectedModule]) {
                    if (categories.empty() || static_cast<size_t>(selectedCategory) >= categories.size() || modules[selectedModule]->GetCategory() == categories[selectedCategory]) {
                        moduleValid = true;
                    }
                    else { // Ha a kiv�lasztott modul nem az aktu�lis kateg�ri�ban van, v�lasszuk az els�t
                        int firstModuleInCurrentCategory = ModuleManager::GetFirstModuleIndexByCategory(categories[selectedCategory]);
                        if (firstModuleInCurrentCategory != -1 && static_cast<size_t>(firstModuleInCurrentCategory) < modules.size() && modules[firstModuleInCurrentCategory]) {
                            if (modules[firstModuleInCurrentCategory]->GetCategory() == categories[selectedCategory]) {
                                selectedModule = firstModuleInCurrentCategory;
                                moduleValid = true;
                            }
                        }
                    }
                }
                else if (!categories.empty() && static_cast<size_t>(selectedCategory) < categories.size()) { // Ha nincs �rv�nyes modul kiv�lasztva, v�lasszuk az els�t
                    int firstModuleInCurrentCategory = ModuleManager::GetFirstModuleIndexByCategory(categories[selectedCategory]);
                    if (firstModuleInCurrentCategory != -1 && static_cast<size_t>(firstModuleInCurrentCategory) < modules.size() && modules[firstModuleInCurrentCategory]) {
                        if (modules[firstModuleInCurrentCategory]->GetCategory() == categories[selectedCategory]) {
                            selectedModule = firstModuleInCurrentCategory;
                            moduleValid = true;
                        }
                    }
                }

                if (moduleValid) {
                    ImGui::PushFont(Menu::fontBold ? Menu::fontBold : ImGui::GetFont());
                    ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x);
                    ImGui::TextColored(ImColor(Colors::accent), "%s", modules[selectedModule]->GetName().c_str());
                    ImGui::PopTextWrapPos();
                    ImGui::PopFont();
                    ImGui::Separator();
                    modules[selectedModule]->RenderMenu();
                }
                else {
                    ImGui::Text("No module selected or module not found in this category.");
                }
            }
            else if (selectedCategory == -1) { // Configs f�l
                if (selectedModule == 0) { // "Local Configs" almen�
                    ImGui::PushFont(Menu::fontBold ? Menu::fontBold : ImGui::GetFont());
                    ImGui::TextColored(ImColor(Colors::accent), "Config Manager");
                    ImGui::PopFont();
                    ImGui::Separator();

                    static std::vector<std::string> configFiles = configmanager::GetConfigList();
                    static int selectedConfig = 0;
                    if (selectedConfig >= static_cast<int>(configFiles.size()) && !configFiles.empty()) selectedConfig = 0;
                    if (configFiles.empty()) selectedConfig = -1;

                    std::string currentSelectedConfigName = (selectedConfig != -1 && static_cast<size_t>(selectedConfig) < configFiles.size()) ? configFiles[selectedConfig] : "None";

                    float listHeight = ImGui::GetContentRegionAvail().y - (ImGui::GetFrameHeightWithSpacing() * 2 + style.ItemSpacing.y + ImGui::GetFrameHeightWithSpacing());
                    listHeight = ImMax(listHeight, 50.0f);

                    ImGui::BeginChild("###ConfigListChild", ImVec2(0, listHeight), true, ImGuiWindowFlags_HorizontalScrollbar);
                    {
                        for (size_t i = 0; i < configFiles.size(); ++i) {
                            bool is_selected = (selectedConfig == static_cast<int>(i));
                            if (ImGui::Selectable((configFiles[i] + "##selectable_cfg_" + std::to_string(i)).c_str(), is_selected, ImGuiSelectableFlags_SpanAllColumns)) {
                                selectedConfig = static_cast<int>(i);
                                setConfigName_flag = std::make_unique<std::once_flag>();
                            }
                            if (is_selected) ImGui::SetItemDefaultFocus();

                            ImGui::SameLine(ImGui::GetContentRegionAvail().x - 60.f - (ImGui::GetCurrentWindowRead()->ScrollbarY ? style.ScrollbarSize : 0.0f));
                            ImGui::PushStyleColor(ImGuiCol_Button, Colors::red_button);
                            if (ImGui::Button(("Delete##cfgdel" + std::to_string(i)).c_str(), ImVec2(60.f, 0))) {
                                std::string removedCfgName = configFiles[i];
                                if (configmanager::RemoveConfig(static_cast<int>(i))) {
                                    NotificationManager::Send("Ida :: Config", "Config \"%s\" has been removed.", removedCfgName.c_str());
                                    configFiles = configmanager::GetConfigList();
                                    if (selectedConfig == static_cast<int>(i)) {
                                        selectedConfig = configFiles.empty() ? -1 : 0;
                                        setConfigName_flag = std::make_unique<std::once_flag>();
                                    }
                                    else if (selectedConfig > static_cast<int>(i) && selectedConfig > 0) {
                                        selectedConfig--;
                                    }
                                }
                                else {
                                    NotificationManager::Send("Ida :: Config", "Config \"%s\" could not be removed.", removedCfgName.c_str());
                                }
                                ImGui::PopStyleColor();
                                break;
                            }
                            ImGui::PopStyleColor();
                        }
                    }
                    ImGui::EndChild(); // ConfigListChild
                    ImGui::Separator();

                    static char saveConfigNameInput[128] = "";
                    std::call_once(*setConfigName_flag, [&]() {
                        if (selectedConfig != -1 && static_cast<size_t>(selectedConfig) < configFiles.size()) {
                            strcpy_s(saveConfigNameInput, sizeof(saveConfigNameInput), configFiles[selectedConfig].c_str());
                        }
                        else {
                            strcpy_s(saveConfigNameInput, sizeof(saveConfigNameInput), "");
                        }
                        });

                    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 90.f - style.ItemSpacing.x);
                    bool saveTriggered = ImGui::InputTextWithHint("##saveconfigname", "Config name...", saveConfigNameInput, IM_ARRAYSIZE(saveConfigNameInput), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CharsNoBlank);
                    ImGui::SameLine(0, style.ItemSpacing.x);
                    if (ImGui::Button("Save", ImVec2(80.f, 0))) saveTriggered = true;

                    if (saveTriggered && strlen(saveConfigNameInput) > 0) {
                        int result = configmanager::SaveConfig(saveConfigNameInput);
                        if (result != -1) {
                            NotificationManager::Send("Ida :: Config", "Config \"%s\" has been saved.", saveConfigNameInput);
                            configFiles = configmanager::GetConfigList();
                            selectedConfig = result;
                            setConfigName_flag = std::make_unique<std::once_flag>();
                        }
                        else {
                            NotificationManager::Send("Ida :: Config", "Config \"%s\" could not be saved.", saveConfigNameInput);
                        }
                    }

                    float buttonWidth = (ImGui::GetContentRegionAvail().x - style.ItemSpacing.x * 2) / 3.0f;
                    buttonWidth = ImMax(buttonWidth, 10.0f);

                    if (ImGui::Button(("Load: " + currentSelectedConfigName).c_str(), ImVec2(buttonWidth, 0))) {
                        if (selectedConfig != -1 && static_cast<size_t>(selectedConfig) < configFiles.size()) {
                            if (configmanager::LoadConfig(selectedConfig)) {
                                NotificationManager::Send("Ida :: Config", "Config \"%s\" has been loaded.", currentSelectedConfigName.c_str());
                                Menu::ResetSetupFlags();
                            }
                            else {
                                NotificationManager::Send("Ida :: Config", "Config \"%s\" could not be loaded.", currentSelectedConfigName.c_str());
                            }
                        }
                    }
                    ImGui::SameLine(0, style.ItemSpacing.x);
                    if (ImGui::Button("Refresh", ImVec2(buttonWidth, 0))) {
                        configFiles = configmanager::GetConfigList();
                        selectedConfig = configFiles.empty() ? -1 : 0;
                        setConfigName_flag = std::make_unique<std::once_flag>();
                        NotificationManager::Send("Ida :: Config", "Config list has been refreshed.");
                    }
                    ImGui::SameLine(0, style.ItemSpacing.x);
                    if (ImGui::Button("Open Folder", ImVec2(buttonWidth, 0))) {
                        std::string configPath = configmanager::GetConfigPath();
                        if (!configPath.empty()) ShellExecuteA(NULL, "open", configPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
                    }
                }
            }
            else if (selectedCategory == -2) { // Settings f�l
                if (selectedModule == 0) { // "General" almen�
                    ImGui::PushFont(Menu::fontBold ? Menu::fontBold : ImGui::GetFont());
                    ImGui::TextColored(ImColor(Colors::accent), "General Settings");
                    ImGui::PopFont();
                    ImGui::Separator();
                    ImGui::Text("Menu Keybind: Insert");
                    ImGui::Text("Detach Keybind: End");
                    ImGui::Separator();
                    ImGui::Checkbox("GUI Movement", &settings::Menu_GUIMovement);
                }
                else if (selectedModule == 1) { // "Friends" almen�
                    ImGui::PushFont(Menu::fontBold ? Menu::fontBold : ImGui::GetFont());
                    ImGui::TextColored(ImColor(Colors::accent), "Friends Management");
                    ImGui::PopFont();
                    ImGui::Separator();
                    std::vector<std::string>& friends = settings::friends;
                    float friendsListHeight = ImGui::GetContentRegionAvail().y - (ImGui::GetFrameHeightWithSpacing() + style.ItemSpacing.y);
                    friendsListHeight = ImMax(friendsListHeight, 50.0f);

                    ImGui::BeginChild("##FriendsListChild", ImVec2(0, friendsListHeight), true, ImGuiWindowFlags_HorizontalScrollbar);
                    {
                        for (size_t i = 0; i < friends.size(); ++i) {
                            ImGui::PushID(static_cast<int>(i));
                            ImGui::Text("%s", friends[i].c_str());
                            ImGui::SameLine(ImGui::GetContentRegionAvail().x - 60.f - (ImGui::GetCurrentWindowRead()->ScrollbarY ? style.ScrollbarSize : 0.0f));
                            ImGui::PushStyleColor(ImGuiCol_Button, Colors::red_button);
                            if (ImGui::Button("Delete", ImVec2(60.f, 0))) {
                                std::string removedFriendName = friends[i];
                                if (configmanager::RemoveFriend(friends[i].c_str())) {
                                    NotificationManager::Send("Ida :: Friends", "Friend \"%s\" has been removed.", removedFriendName.c_str());
                                }
                                else {
                                    NotificationManager::Send("Ida :: Friends", "Friend \"%s\" could not be removed.", removedFriendName.c_str());
                                }
                                ImGui::PopStyleColor();
                                ImGui::PopID();
                                break;
                            }
                            ImGui::PopStyleColor();
                            ImGui::PopID();
                            if (i < friends.size() - 1 && friends.size() > 1) ImGui::Separator();
                        }
                    }
                    ImGui::EndChild(); // FriendsListChild

                    static char newFriendName[128] = "";
                    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 90.f - style.ItemSpacing.x);
                    bool addFriendTriggered = ImGui::InputTextWithHint("##addFriendName", "Enter friend's name...", newFriendName, IM_ARRAYSIZE(newFriendName), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CharsNoBlank);
                    ImGui::SameLine(0, style.ItemSpacing.x);
                    ImGui::PushStyleColor(ImGuiCol_Button, Colors::green_button);
                    if (ImGui::Button("Add", ImVec2(80.f, 0))) addFriendTriggered = true;

                    if (addFriendTriggered && strlen(newFriendName) > 0) {
                        if (configmanager::AddFriend(newFriendName)) {
                            NotificationManager::Send("Ida :: Friends", "Friend \"%s\" has been added.", newFriendName);
                            strcpy_s(newFriendName, "");
                        }
                        else {
                            NotificationManager::Send("Ida :: Friends", "Friend \"%s\" could not be added.", newFriendName);
                        }
                    }
                    ImGui::PopStyleColor();
                }
                else if (selectedModule == 2) { // "HUD" almen�
                    ImGui::PushFont(Menu::fontBold ? Menu::fontBold : ImGui::GetFont());
                    ImGui::TextColored(ImColor(Colors::accent), "HUD Settings");
                    ImGui::PopFont();
                    ImGui::Separator();
                    ImGui::Checkbox("Disable all HUD/Overlay rendering", &settings::Hud_DisableAllRendering);
                    ImGui::Separator();
                    ImGui::Text("Watermark Settings:"); ImGui::Spacing();
                    ImGui::Checkbox("Show Watermark", &settings::Hud_Watermark);
                    if (settings::Hud_Watermark) {
                        ImGui::Indent();
                        ImGui::Checkbox("Show Version", &settings::Hud_WatermarkVersion);
                        ImGui::Checkbox("Show FPS", &settings::Hud_WatermarkFps);
                        ImGui::Checkbox("Show Ping", &settings::Hud_WatermarkPing);
                        ImGui::Checkbox("Show Coordinates", &settings::Hud_WatermarkCoords);
                        ImGui::Checkbox("Show Direction", &settings::Hud_WatermarkDirection);
                        ImGui::Checkbox("Show Time", &settings::Hud_WatermarkTime);
                        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.7f);
                        if (ImGui::SliderFloat("X Position##Watermark", &settings::Hud_WatermarkPosition[0], 0.f, 1920.f, "%.0f")) Menu::ResetSetupFlags();
                        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.7f);
                        if (ImGui::SliderFloat("Y Position##Watermark", &settings::Hud_WatermarkPosition[1], 0.f, 1080.f, "%.0f")) Menu::ResetSetupFlags();
                        ImGui::Unindent();
                    }
                    ImGui::Separator();
                    ImGui::Text("Keybinds List Settings:"); ImGui::Spacing();
                    ImGui::Checkbox("Show Keybinds List", &settings::Hud_ShowKeybinds);
                    if (settings::Hud_ShowKeybinds) {
                        ImGui::Indent();
                        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.7f);
                        if (ImGui::SliderFloat("X Position##Keybinds", &settings::Hud_KeybindsPosition[0], 0.f, 1920.f, "%.0f")) Menu::ResetSetupFlags();
                        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.7f);
                        if (ImGui::SliderFloat("Y Position##Keybinds", &settings::Hud_KeybindsPosition[1], 0.f, 1080.f, "%.0f")) Menu::ResetSetupFlags();
                        ImGui::Unindent();
                    }
                    ImGui::Separator();
                    ImGui::Text("Radar Settings:"); ImGui::Spacing();
                    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.7f);
                    if (ImGui::SliderFloat("X Position##Radar", &settings::Radar_Position[0], 0.f, 1920.f, "%.0f")) Menu::ResetSetupFlags();
                    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.7f);
                    if (ImGui::SliderFloat("Y Position##Radar", &settings::Radar_Position[1], 0.f, 1080.f, "%.0f")) Menu::ResetSetupFlags();
                    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.7f);
                    if (ImGui::SliderFloat("Size##Radar", &settings::Radar_Size, 50.f, 500.f, "%.0f")) Menu::ResetSetupFlags();
                    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.7f);
                    ImGui::SliderFloat("Rounding##Radar", &settings::Radar_SquareRoundness, 0.f, 100.f, "%.0f");
                }
            }
        }
        ImGui::EndChild(); // RightPanel_ContentArea
        ImGui::PopStyleVar(); // RightPanel bels� padding
    }
    ImGui::End();

    style.Colors[ImGuiCol_ChildBg] = originalChildBg;
    ImGui::PopStyleVar(9);
    ImGui::PopStyleColor(17);
}