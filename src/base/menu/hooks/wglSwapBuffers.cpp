#include "menu/menu.h"

#include <mutex>

#include <gl/GL.h>
#include <Shlobj.h>

#include <imgui/imgui_impl_opengl2.h>
#include <imgui/imgui_impl_win32.h>
#include <imgui/imgui_internal.h>
#include <minhook/minhook.h>

#include "util/logger.h"
#include "configManager/settings.h"
#include "base.h"

#include "fonts/inter.h"

std::once_flag setupFlag;
std::atomic_flag clipCursor = ATOMIC_FLAG_INIT;
RECT originalClip;

typedef bool(__stdcall* template_wglSwapBuffers) (HDC hdc);
template_wglSwapBuffers original_wglSwapBuffers;
bool __stdcall hook_wglSwapBuffers(_In_ HDC hdc)
{
	// handling fullscreen context switching before we set the new hwnd, so we can compare them
	
	// if the cached hwnd isnt equal to the current one
	// info: window handles change when you enter/exit fullscreen
	if (Menu::handleWindow != WindowFromDC(hdc) && Menu::initialized)
	{
		// set handlewindow so that the wndproc can attach to new one before passing
		Menu::handleWindow = WindowFromDC(hdc);

		// opengl context for font issue
		Menu::originalGLContext = wglGetCurrentContext();
		wglMakeCurrent(hdc, wglCreateContext(hdc));

		// uninitialize imgui opengl and win32 implementation
		ImGui_ImplOpenGL2_Shutdown();
		ImGui_ImplWin32_Shutdown();
		
		// reinitialize it
		ImGui_ImplWin32_Init(Menu::handleWindow);
		ImGui_ImplOpenGL2_Init();

		// set wndproc
		Menu::Hook_wndProc();

		// revert to old ctx
		wglMakeCurrent(hdc, Menu::originalGLContext);

		// end detour
		return original_wglSwapBuffers(hdc);
	}

	Menu::handleDeviceContext = hdc;
	Menu::handleWindow = WindowFromDC(hdc);
	Menu::originalGLContext = wglGetCurrentContext();

	std::call_once(setupFlag, [&] {
		Menu::Hook_wndProc();
		Menu::SetupImgui();
	});

	wglMakeCurrent(Menu::handleDeviceContext, Menu::menuGLContext);

	ImGui_ImplOpenGL2_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	ImGui::PushFont(Menu::font);

	if (Menu::open)
	{
		if (clipCursor.test_and_set()) GetClipCursor(&originalClip);

		ClipCursor(NULL);
		Menu::RenderMenu();
	}
	else
	{
		// checking if originalClip is valid
		if (originalClip.right > originalClip.left && originalClip.bottom > originalClip.top)
		{
			ClipCursor(&originalClip);
			clipCursor.clear();
		}
	}

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGui::SetNextWindowPos(ImVec2(0, 0));
	ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
	ImGui::Begin("Overlay", nullptr,
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoInputs |
		ImGuiWindowFlags_NoFocusOnAppearing |
		ImGuiWindowFlags_NoBringToFrontOnFocus |
		ImGuiWindowFlags_NoBackground);

	Base::renderLoop();

	ImGui::PopFont();
	ImGui::End();
	ImGui::PopStyleVar();

	ImGui::EndFrame();
	ImGui::Render();
	ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());

	wglMakeCurrent(Menu::handleDeviceContext, Menu::originalGLContext);
	return original_wglSwapBuffers(hdc);
}

void Menu::Hook_wglSwapBuffers()
{
	HMODULE opengl32Handle = GetModuleHandle("opengl32.dll");
	if (opengl32Handle == 0)
	{
		LOG_ERROR("Failed to find \"opengl32.dll\".");
		return;
	}

	LPVOID wglSwapBuffers = (LPVOID)GetProcAddress(opengl32Handle, "wglSwapBuffers");
	MH_CreateHook(wglSwapBuffers, (LPVOID)hook_wglSwapBuffers, (LPVOID*)&original_wglSwapBuffers);
	MH_EnableHook(wglSwapBuffers);
}

void Menu::Unhook_wglSwapBuffers()
{	
	MH_DisableHook(MH_ALL_HOOKS);
	MH_RemoveHook(MH_ALL_HOOKS);
}

void Menu::SetupImgui()
{
	Menu::menuGLContext = wglCreateContext(Menu::handleDeviceContext);
	wglMakeCurrent(Menu::handleDeviceContext, Menu::menuGLContext);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	GLint m_viewport[4];
	glGetIntegerv(GL_VIEWPORT, m_viewport);

	glOrtho(0, m_viewport[2], m_viewport[3], 0, 1, -1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glClearColor(0, 0, 0, 0);

	Menu::currentImGuiContext = ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.Fonts->AddFontDefault();

	Menu::font = io.Fonts->AddFontFromMemoryTTF(inter_regular, sizeof(inter_regular), 16);
	Menu::fontBold = io.Fonts->AddFontFromMemoryTTF(inter_bold, sizeof(inter_bold), 24);

	Menu::font28 = io.Fonts->AddFontFromMemoryTTF(inter_regular, sizeof(inter_regular), 28);
	Menu::font26 = io.Fonts->AddFontFromMemoryTTF(inter_regular, sizeof(inter_regular), 26);
	Menu::font24 = io.Fonts->AddFontFromMemoryTTF(inter_regular, sizeof(inter_regular), 24);
	Menu::font22 = io.Fonts->AddFontFromMemoryTTF(inter_regular, sizeof(inter_regular), 22);
	Menu::font20 = io.Fonts->AddFontFromMemoryTTF(inter_regular, sizeof(inter_regular), 20);
	Menu::font18 = io.Fonts->AddFontFromMemoryTTF(inter_regular, sizeof(inter_regular), 18);
	Menu::font16 = io.Fonts->AddFontFromMemoryTTF(inter_regular, sizeof(inter_regular), 16);
	Menu::font14 = io.Fonts->AddFontFromMemoryTTF(inter_regular, sizeof(inter_regular), 14);

	Menu::boldFont28 = io.Fonts->AddFontFromMemoryTTF(inter_bold, sizeof(inter_bold), 28);
	Menu::boldFont26 = io.Fonts->AddFontFromMemoryTTF(inter_bold, sizeof(inter_bold), 26);
	Menu::boldFont24 = io.Fonts->AddFontFromMemoryTTF(inter_bold, sizeof(inter_bold), 24);
	Menu::boldFont22 = io.Fonts->AddFontFromMemoryTTF(inter_bold, sizeof(inter_bold), 22);
	Menu::boldFont20 = io.Fonts->AddFontFromMemoryTTF(inter_bold, sizeof(inter_bold), 20);
	Menu::boldFont18 = io.Fonts->AddFontFromMemoryTTF(inter_bold, sizeof(inter_bold), 18);
	Menu::boldFont16 = io.Fonts->AddFontFromMemoryTTF(inter_bold, sizeof(inter_bold), 16);
	Menu::boldFont14 = io.Fonts->AddFontFromMemoryTTF(inter_bold, sizeof(inter_bold), 14);

	SetupStyle();

	ImGui_ImplWin32_Init(Menu::handleWindow);
	ImGui_ImplOpenGL2_Init();

	Menu::initialized = true;
}

void Menu::SetupStyle()
{
	ImVec4* colors = ImGui::GetStyle().Colors;
	colors[ImGuiCol_Text] = ImVec4(settings::Menu_TextColor[0], settings::Menu_TextColor[1], settings::Menu_TextColor[2], settings::Menu_TextColor[3]);
	colors[ImGuiCol_WindowBg] = ImVec4(settings::Menu_BackgroundColor[0], settings::Menu_BackgroundColor[1], settings::Menu_BackgroundColor[2], settings::Menu_BackgroundColor[3]);
	colors[ImGuiCol_ChildBg] = ImVec4(settings::Menu_ChildBackgroundColor[0], settings::Menu_ChildBackgroundColor[1], settings::Menu_ChildBackgroundColor[2], settings::Menu_ChildBackgroundColor[3]);
	colors[ImGuiCol_Border] = ImVec4(settings::Menu_OutlineColor[0], settings::Menu_OutlineColor[1], settings::Menu_OutlineColor[2], settings::Menu_OutlineColor[3]);
	colors[ImGuiCol_PopupBg] = ImVec4(settings::Menu_ChildBackgroundColor[0], settings::Menu_ChildBackgroundColor[1], settings::Menu_ChildBackgroundColor[2], settings::Menu_ChildBackgroundColor[3]); // Popup-ok is legyenek sötétek

	// Gombok (általános) - most vörös lesz a Menu_PrimaryColor miatt
	colors[ImGuiCol_Button] = ImVec4(settings::Menu_PrimaryColor[0], settings::Menu_PrimaryColor[1], settings::Menu_PrimaryColor[2], settings::Menu_PrimaryColor[3]);
	colors[ImGuiCol_ButtonHovered] = ImVec4(ImClamp<float>(settings::Menu_PrimaryColor[0] * 1.2f, 0.0f, 1.0f), ImClamp<float>(settings::Menu_PrimaryColor[1] * 1.2f, 0.0f, 1.0f), ImClamp<float>(settings::Menu_PrimaryColor[2] * 1.2f, 0.0f, 1.0f), settings::Menu_PrimaryColor[3]); // Világosabb vörös hover
	colors[ImGuiCol_ButtonActive] = ImVec4(ImClamp<float>(settings::Menu_PrimaryColor[0] * 0.8f, 0.0f, 1.0f), ImClamp<float>(settings::Menu_PrimaryColor[1] * 0.8f, 0.0f, 1.0f), ImClamp<float>(settings::Menu_PrimaryColor[2] * 0.8f, 0.0f, 1.0f), settings::Menu_PrimaryColor[3]); // Sötétebb vörös active

	// Keretek (input mezõk, csúszkák háttere stb.) - közepes szürke
	float frameBgGray = 0.25f;
	colors[ImGuiCol_FrameBg] = ImVec4(frameBgGray, frameBgGray, frameBgGray, 1.0f);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(frameBgGray * 1.2f, frameBgGray * 1.2f, frameBgGray * 1.2f, 1.0f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(frameBgGray * 0.8f, frameBgGray * 0.8f, frameBgGray * 0.8f, 1.0f);

	// Csúszka és Pipa - vörös lesz a Menu_SecondaryColor (ami most szintén vörös) miatt
	colors[ImGuiCol_SliderGrab] = ImVec4(settings::Menu_SecondaryColor[0], settings::Menu_SecondaryColor[1], settings::Menu_SecondaryColor[2], settings::Menu_SecondaryColor[3]);
	colors[ImGuiCol_SliderGrabActive] = ImVec4(ImClamp<float>(settings::Menu_SecondaryColor[0] * 0.8f, 0.0f, 1.0f), ImClamp<float>(settings::Menu_SecondaryColor[1] * 0.8f, 0.0f, 1.0f), ImClamp<float>(settings::Menu_SecondaryColor[2] * 0.8f, 0.0f, 1.0f), settings::Menu_SecondaryColor[3]);
	colors[ImGuiCol_CheckMark] = ImVec4(settings::Menu_SecondaryColor[0], settings::Menu_SecondaryColor[1], settings::Menu_SecondaryColor[2], settings::Menu_SecondaryColor[3]);

	// Scrollbar - sötét alapon közepes szürke fogantyú
	colors[ImGuiCol_ScrollbarBg] = ImVec4(settings::Menu_BackgroundColor[0], settings::Menu_BackgroundColor[1], settings::Menu_BackgroundColor[2], settings::Menu_BackgroundColor[3]);
	colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.4f, 0.4f, 0.4f, 1.0f);
	colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
	colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);

	// Fejlécek (pl. kategória gombok, tabok) - Sötétszürke alapon vörös aktív/hover
	colors[ImGuiCol_Header] = ImVec4(0.2f, 0.2f, 0.2f, 1.0f); // Sötétszürke alap
	colors[ImGuiCol_HeaderHovered] = ImVec4(settings::Menu_PrimaryColor[0], settings::Menu_PrimaryColor[1], settings::Menu_PrimaryColor[2], 0.8f); // Vörös hover (enyhe áttetszõséggel, vagy 1.0f)
	colors[ImGuiCol_HeaderActive] = ImVec4(settings::Menu_PrimaryColor[0], settings::Menu_PrimaryColor[1], settings::Menu_PrimaryColor[2], 1.0f); // Teljesen vörös aktív

	// Elválasztók
	colors[ImGuiCol_Separator] = ImVec4(settings::Menu_SeperatorColor[0], settings::Menu_SeperatorColor[1], settings::Menu_SeperatorColor[2], settings::Menu_SeperatorColor[3]);
	// ImGuiCol_SeparatorHovered és ImGuiCol_SeparatorActive általában nem használtak, de ha igen:
	// colors[ImGuiCol_SeparatorHovered] = ImVec4( ... );
	// colors[ImGuiCol_SeparatorActive] = ImVec4( ... );


	ImGuiStyle& style = ImGui::GetStyle();
	style.WindowRounding = settings::Menu_WindowRounding;
	style.ChildRounding = settings::Menu_WindowRounding; // Legyen konzisztens
	style.FrameRounding = settings::Menu_ComponentsRounding;
	style.ScrollbarRounding = settings::Menu_ComponentsRounding;
	style.GrabRounding = settings::Menu_ComponentsRounding;
	style.PopupRounding = settings::Menu_WindowRounding; // Popupok is illeszkedjenek

	style.WindowPadding = ImVec2(10.f, 10.f);
	style.ItemSpacing = ImVec2(10.f, 10.f);      // Kicsit több vertikális hely az elemek között (opcionális)
	style.ItemInnerSpacing = ImVec2(5.f, 5.f);
	style.FramePadding = ImVec2(8.f, 6.f);      // Kicsit magasabbak az input mezõk, gombok (opcionális)

	// Nincs többé áttetszõség az ablakon vagy a kereteken
	style.WindowMenuButtonPosition = ImGuiDir_None; // Eltünteti a régi debug menü gombot
	style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	style.Colors[ImGuiCol_ChildBg].w = 1.0f;
	style.Colors[ImGuiCol_PopupBg].w = 1.0f;
}
