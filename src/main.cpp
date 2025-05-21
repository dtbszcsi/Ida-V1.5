#include "main.h"    // Ennek kell tartalmaznia a 'struct Main' vagy 'class Main' deklarációját,
// beleértve a 'static HMODULE hModule;', 'static void Init();', 'static void Shutdown();' sorokat.
#include "base/base.h" // A Base::Init() és Base::Shutdown() hívásokhoz

// Login UI-hoz szükséges include-ok (ezek a te kódodból valók)
#include "LoginUI.h"
#include "base/menu/menu.h" 

// Szükséges include-ok a HWID ellenõrzéshez és WMI-hez
#include <string>    // std::string használatához
#include <vector>    // std::vector használatához
#include <windows.h> // Alap Windows API-k (lehet, hogy a main.h már tartalmazza, de biztos, ami biztos)
#include <comdef.h>  // _bstr_t, _variant_t a WMI-hez
#include <wbemidl.h> // WMI interfészek
#pragma comment(lib, "wbemuuid.lib") // WMI könyvtár linkelése

// Finomított WMI-alapú függvény a fizikai merevlemez sorozatszámának lekérdezéséhez
std::string GetCurrentMachineHWID() {
    std::string diskSerial = "UNKNOWN_HWID"; // Alapértelmezett érték hiba esetére
    HRESULT hr;

    HRESULT hrCoInit = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    bool weArePrimaryInitializer = (hrCoInit == S_OK);
    bool canProceedWithCOM = (hrCoInit == S_OK || hrCoInit == S_FALSE);

    if (hrCoInit == RPC_E_CHANGED_MODE) {
        return "HWID_ERR_COM_MODE_CONFLICT";
    }
    if (!canProceedWithCOM) {
        return "HWID_ERR_COM_INIT_FAILED";
    }

    hr = S_OK;

    HRESULT hrSec = CoInitializeSecurity(
        NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);

    if (FAILED(hrSec) && hrSec != RPC_E_TOO_LATE) {
        if (weArePrimaryInitializer) CoUninitialize();
        return "HWID_ERR_COM_SEC";
    }

    IWbemLocator* pLoc = NULL;
    hr = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID*)&pLoc);
    if (FAILED(hr)) {
        if (weArePrimaryInitializer) CoUninitialize();
        return "HWID_ERR_LOC_INSTANCE";
    }

    IWbemServices* pSvc = NULL;
    hr = pLoc->ConnectServer(_bstr_t(L"ROOT\\CIMV2"), NULL, NULL, 0, NULL, 0, 0, &pSvc);
    if (FAILED(hr)) {
        pLoc->Release();
        if (weArePrimaryInitializer) CoUninitialize();
        return "HWID_ERR_CONNECT_SERVER";
    }

    hr = CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);
    if (FAILED(hr)) {
        pSvc->Release();
        pLoc->Release();
        if (weArePrimaryInitializer) CoUninitialize();
        return "HWID_ERR_PROXY_BLANKET";
    }

    IEnumWbemClassObject* pEnumerator = NULL;
    hr = pSvc->ExecQuery(bstr_t("WQL"), bstr_t("SELECT SerialNumber FROM Win32_DiskDrive"), WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pEnumerator);
    if (FAILED(hr)) {
        pSvc->Release();
        pLoc->Release();
        if (weArePrimaryInitializer) CoUninitialize();
        return "HWID_ERR_WMI_QUERY";
    }

    IWbemClassObject* pclsObj = NULL;
    ULONG uReturn = 0;
    if (pEnumerator && pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn) == S_OK && uReturn != 0) {
        VARIANT vtProp;
        VariantInit(&vtProp);
        hr = pclsObj->Get(L"SerialNumber", 0, &vtProp, 0, 0);
        if (SUCCEEDED(hr) && vtProp.vt == VT_BSTR && vtProp.bstrVal != NULL) {
            std::wstring wSerial = vtProp.bstrVal;
            std::wstring cleanedSerial;
            for (wchar_t wc : wSerial) {
                if (wc != L' ' && wc != L'\t' && wc != L'\n' && wc != L'\r') {
                    cleanedSerial += wc;
                }
            }
            if (!cleanedSerial.empty()) {
                int size_needed = WideCharToMultiByte(CP_UTF8, 0, cleanedSerial.c_str(), (int)cleanedSerial.length(), NULL, 0, NULL, NULL);
                std::string serialStr(size_needed, 0);
                WideCharToMultiByte(CP_UTF8, 0, cleanedSerial.c_str(), (int)cleanedSerial.length(), &serialStr[0], size_needed, NULL, NULL);
                diskSerial = "DISK_SN_" + serialStr;
            }
            else {
                diskSerial = "HWID_WARN_EMPTY_SERIAL";
            }
        }
        else {
            diskSerial = "HWID_ERR_GET_SERIAL_PROP";
        }
        VariantClear(&vtProp);
        pclsObj->Release();
    }
    else {
        diskSerial = "HWID_WARN_NO_DISK_FOUND";
    }

    if (pEnumerator) pEnumerator->Release();
    pSvc->Release();
    pLoc->Release();

    if (weArePrimaryInitializer) {
        CoUninitialize();
    }
    return diskSerial;
}

// Manuálisan megadott, engedélyezett HWID-k listája
// Itt van a te HWID-ed. Ha többet szeretnél, add hozzá vesszõvel elválasztva.
const std::vector<std::string> allowed_hwids = {
    "DISK_SN_8E310777030800443241" // A te HWID-ed
    // Például:
    // , "DISK_SN_MASIKHWIDXYZ123"
};

// Globális állapotváltozók a bejelentkezési rendszerhez (a te kódodból)
bool g_IsUserAuthenticated = false;
char g_UsernameInputBuffer[128] = "";
char g_PasswordInputBuffer[128] = "";

// A 'Main::hModule' statikus tagváltozó DEFINÍCIÓJA (a te kódodból)
HMODULE Main::hModule = nullptr;

// A Main struktúra/osztály statikus metódusainak implementációja (a te kódodból)
void Main::Init()
{
    Base::Init();
}

void Main::Shutdown()
{
    Base::Shutdown();
    if (Main::hModule != nullptr) {
        FreeLibraryAndExitThread(Main::hModule, 0);
    }
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        // --- HWID Ellenõrzés Kezdete ---
        std::string current_hwid = GetCurrentMachineHWID();

        // Debug célra (késõbb töröld vagy kommentezd ki!):
        // std::string debug_msg = "Aktuális gép HWID-je:\n" + current_hwid + "\n\nEngedélyezett HWID-k a listában:\n";
        // for(const auto& id : allowed_hwids) { debug_msg += id + "\n"; }
        // MessageBoxA(NULL, debug_msg.c_str(), "HWID Info (DEBUG)", MB_OK | MB_ICONINFORMATION);

        bool is_hwid_allowed = false;
        if (!current_hwid.empty() && current_hwid.rfind("HWID_ERR", 0) != 0 && current_hwid.rfind("HWID_WARN", 0) != 0) {
            for (const std::string& allowed_id_in_list : allowed_hwids) {
                if (current_hwid == allowed_id_in_list) {
                    is_hwid_allowed = true;
                    break;
                }
            }
        }
        else {
            // Hiba történt a HWID lekérdezésekor vagy érvénytelen eredmény.
            // MessageBoxA(NULL, ("HWID lekérdezési hiba/figyelmeztetés: " + current_hwid).c_str(), "HWID Hiba", MB_OK | MB_ICONERROR);
            is_hwid_allowed = false;
        }

        if (!is_hwid_allowed) {
            // MessageBoxA(NULL, "HWID ellenõrzés sikertelen! A DLL nem töltõdik be.", "Hozzáférés Megtagadva", MB_OK | MB_ICONERROR);
            return FALSE; // HWID nem egyezik, vagy hiba történt -> DLL betöltés megakadályozása
        }
        // --- HWID Ellenõrzés Vége ---

        // Ha idáig eljutottunk, a HWID rendben van, folytatódhat az eredeti DllMain logika:
        Main::hModule = hinstDLL;
        DisableThreadLibraryCalls(hinstDLL);

        HANDLE hThread = CreateThread(
            nullptr,
            0,
            reinterpret_cast<LPTHREAD_START_ROUTINE>(Main::Init),
            nullptr,
            0,
            nullptr
        );

        if (hThread) {
            CloseHandle(hThread);
        }
        else {
            // Hiba történt a szál létrehozásakor (még a sikeres HWID ellenõrzés után is).
            // MessageBoxA(NULL, "Nem sikerült létrehozni a Main::Init szálat!", "Kritikus Hiba", MB_OK | MB_ICONERROR);
            return FALSE; // DLL betöltés sikertelen
        }
    }
    else if (fdwReason == DLL_PROCESS_DETACH)
    {
        // Takarítás, ha szükséges (a te eredeti kódod szerint)
    }

    return TRUE; // Sikeres DLL mûvelet
}