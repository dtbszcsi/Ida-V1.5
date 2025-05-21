// LoginUI.cpp - Modern ImGui login interface implementation
// ... (a f�jl t�bbi r�sze v�ltozatlan marad, csak a RenderLoginWindow relev�ns r�szeit mutatom) ...

#include "LoginUI.h"
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <cstring>
#include <string>
#include <map>

// ... (static variables �s user_credentials v�ltozatlan) ...
static const std::map<std::string, std::string> user_credentials = {
    {"ilovemycash", "OOOOO"},
    {"Pxchito", "PiSOMbERIC"},
    {"KataNeni", "K@t@J3lsz0"},
};
static char loginMessage[128] = "";
static bool showPassword = false;

namespace LoginUI {

    static void TextCentered(const std::string& text) {
        float win_width = ImGui::GetWindowSize().x;
        float text_width = ImGui::CalcTextSize(text.c_str()).x;
        ImGui::SetCursorPosX((win_width - text_width) * 0.5f);
        ImGui::Text("%s", text.c_str());
    }

    void RenderLoginWindow(bool& isAuthenticated, char* usernameBuffer, int usernameBufferSize, char* passwordBuffer, int passwordBufferSize) {
        if (isAuthenticated) {
            return;
        }

        ImGuiIO& io = ImGui::GetIO();
        ImVec2 displaySize = io.DisplaySize;
        ImVec2 windowSize = ImVec2(400, 0);

        ImGui::SetNextWindowSize(windowSize, ImGuiCond_Appearing);
        ImGui::SetNextWindowPos(ImVec2(displaySize.x * 0.5f, displaySize.y * 0.5f), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(25.0f, 25.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.0f, 7.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10.0f, 10.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize, 20.0f);

        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.1f, 0.105f, 0.11f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.205f, 0.21f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.25f, 0.255f, 0.26f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.28f, 0.285f, 0.29f, 1.00f));

        // C�msor sz�neinek m�dos�t�sa (nem z�ld)
        ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.08f, 0.08f, 0.15f, 1.00f));          // S�t�tebb k�k/lila alapsz�n a c�msornak
        ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.15f, 0.15f, 0.25f, 1.00f));    // Kicsit �l�nkebb k�k/lila, ha akt�v
        ImGui::PushStyleColor(ImGuiCol_TitleBgCollapsed, ImVec4(0.08f, 0.08f, 0.15f, 1.00f)); // Ha �sszecsukhat� lenne

        ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0.0f, 0.7f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.45f, 0.9f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.05f, 0.55f, 1.0f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 0.4f, 0.8f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.0f, 0.45f, 0.9f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.05f, 0.55f, 1.0f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.0f, 0.4f, 0.8f, 1.0f));

        // Most 14 PushStyleColor h�v�s van (az el�z� 12-b�l egyet cser�lt�nk 3-ra)

        if (ImGui::Begin("Login to Ida. V1", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize)) {

            ImGui::Dummy(ImVec2(0.0f, 5.0f));
            TextCentered("Welcome Back!");
            ImGui::Dummy(ImVec2(0.0f, 5.0f));
            TextCentered("Please enter your credentials to continue.");
            ImGui::Dummy(ImVec2(0.0f, 15.0f));

            ImGui::SeparatorText("Credentials");
            ImGui::Dummy(ImVec2(0.0f, 10.0f));

            ImGui::Text("Username");
            ImGui::PushItemWidth(-1);
            if (ImGui::InputText("##Username", usernameBuffer, usernameBufferSize, ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_EnterReturnsTrue)) {
                ImGui::SetKeyboardFocusHere(1);
            }
            ImGui::PopItemWidth();
            ImGui::Dummy(ImVec2(0.0f, 5.0f));

            ImGui::Text("Password");
            ImGui::PushItemWidth(-1);
            ImGuiInputTextFlags passwordFlags = ImGuiInputTextFlags_EnterReturnsTrue;
            if (!showPassword) {
                passwordFlags |= ImGuiInputTextFlags_Password;
            }
            bool loginAttempt = ImGui::InputText("##Password", passwordBuffer, passwordBufferSize, passwordFlags);
            ImGui::PopItemWidth();

            ImGui::SameLine(0, 5.0f);
            ImGui::Checkbox("Show", &showPassword);

            ImGui::Dummy(ImVec2(0.0f, 15.0f));

            bool buttonClicked = ImGui::Button("Login", ImVec2(ImGui::GetContentRegionAvail().x, 45));

            if (loginAttempt || buttonClicked) {
                std::string currentUsername = usernameBuffer;
                std::string currentPassword = passwordBuffer;

                auto it = user_credentials.find(currentUsername);
                if (it != user_credentials.end()) {
                    if (it->second == currentPassword) {
                        isAuthenticated = true;
                        usernameBuffer[0] = '\0';
                        passwordBuffer[0] = '\0';
                        loginMessage[0] = '\0';
                        showPassword = false;
                    }
                    else {
                        strcpy_s(loginMessage, sizeof(loginMessage), "Invalid username or password!");
                        passwordBuffer[0] = '\0';
                    }
                }
                else {
                    strcpy_s(loginMessage, sizeof(loginMessage), "Invalid username or password!");
                    passwordBuffer[0] = '\0';
                }
            }

            if (loginMessage[0] != '\0' && !isAuthenticated) {
                ImGui::Dummy(ImVec2(0.0f, 10.0f));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
                TextCentered(loginMessage);
                ImGui::PopStyleColor();
            }

            ImGui::Dummy(ImVec2(0.0f, 10.0f));
            ImGui::End();
        }

        ImGui::PopStyleColor(14); // Mivel 14 PushStyleColor volt
        ImGui::PopStyleVar(6);
    }

} // End of LoginUI namespace