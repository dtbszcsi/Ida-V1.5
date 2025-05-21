#pragma once

// LoginUI.h - Modern ImGui login interface

// Forward declarations for ImGui types (no need to include the full header here)
struct ImVec2;
struct ImVec4;

namespace LoginUI {
    // Funkci� a login ablak megjelen�t�s�re �s a hiteles�t�s kezel�s�re.
    // isAuthenticated: Referencia egy bool v�ltoz�ra, ami a bejelentkez�si �llapotot t�rolja.
    // usernameBuffer, passwordBuffer: Pufferek a beviteli mez�k tartalm�nak.
    void RenderLoginWindow(bool& isAuthenticated, char* usernameBuffer, int usernameBufferSize, char* passwordBuffer, int passwordBufferSize);
}
