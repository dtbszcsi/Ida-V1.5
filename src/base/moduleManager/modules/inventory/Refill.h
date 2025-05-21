#pragma once
// Sz�ks�ges header�k
#include "moduleManager/moduleBase.h"    // Az �soszt�ly defin�ci�ja (ellen�rizd az el�r�si utat!)
#include "configManager/settings.h" // A settings::RF_Key �s settings::RF_Enabled el�r�s�hez (ellen�rizd az el�r�si utat!)
#include <string>                   // std::string haszn�lat�hoz (m_name, m_category)
#include <vector>                   // std::vector haszn�lat�hoz (m_potionSlots, m_emptyHotbarSlots)
#include <chrono>                   // std::chrono haszn�lat�hoz (id�z�t�k)

class Refill : public ModuleBase
{
public:
    // Konstruktor
    Refill() :
        ModuleBase(), // Az �soszt�ly (ModuleBase) alap�rtelmezett konstruktor�nak h�v�sa
        m_name("Refill"),
        m_category("Inventory"),
        m_isRefilling(false),
        m_inventorySlotIndex(0),
        m_waitingForSecondClick_Refill(false),
        m_lastRefillTime(std::chrono::steady_clock::time_point()),
        m_shouldOpenInventory(false),
        m_shouldCloseInventory(false),
        m_waitingForInventorySettle(false),
        m_inventorySettleTimer(std::chrono::steady_clock::time_point()),
        m_inventoryOpenAttempts(0),
        m_lastInventoryOpenAttempt(std::chrono::steady_clock::time_point())
        // m_lastContinuousRefillTime a default member initializer seg�ts�g�vel inicializ�l�dik
        // A vektorok (m_potionSlots, m_emptyHotbarSlots) alap�rtelmezetten �resek lesznek.
    {
    }

    // --- A ModuleBase �ltal el��rt, fel�l�rand� virtu�lis met�dusok ---
    void Update() override;
    void RenderMenu() override;

    // A p�lda alapj�n ezeket is implement�lni kell (ak�r �resen, ha a Refill nem haszn�lja �ket)
    void RenderOverlay() override { /* Refill nem haszn�lja, vagy ide j�n a specifikus k�d */ }
    void RenderHud() override { /* Refill nem haszn�lja, vagy ide j�n a specifikus k�d */ }
    std::string GetName() override { return m_name; }
    std::string GetCategory() override { return m_category; }

    // A Refill saj�t be�ll�t�sait haszn�lva
    int GetKey() override { return settings::RF_Key; }
    bool IsEnabled() override { return settings::RF_Enabled; }
    void SetEnabled(bool enabled) override { settings::RF_Enabled = enabled; }
    void Toggle() override { settings::RF_Enabled = !settings::RF_Enabled; }

    // --- A Refill oszt�ly saj�t publikus met�dusai (NEM override-olnak semmit a ModuleBase-b�l) ---
    // Ezt a met�dust a Refill::Update() h�vja meg a billenty�zetfigyel�s alapj�n.
    void OnKeybind();

private:
    // Bels� seg�df�ggv�nyek a refill logik�j�hoz
    void FindPotions();
    void FindEmptyHotbarSlots();
    void ResetRefill();
    void OpenInventory(); // �j f�ggv�ny az inventory automatikus megnyit�s�hoz
    // �j f�ggv�ny a folyamatos m�dhoz
    void CheckForContinuousRefill(); // Ellen�rzi, kell-e �j refill ciklust ind�tani

    // Modul azonos�t�i (a ModuleBase nem a konstruktorban v�rja �ket)
    std::string m_name;
    std::string m_category;

    // Refill m�k�d�si �llapotai
    bool m_isRefilling;
    int m_inventorySlotIndex;
    bool m_waitingForSecondClick_Refill;
    std::chrono::steady_clock::time_point m_lastRefillTime;

    // Inventory kezel�ssel kapcsolatos �llapotok
    bool m_shouldOpenInventory;
    bool m_shouldCloseInventory;
    bool m_waitingForInventorySettle;
    std::chrono::steady_clock::time_point m_inventorySettleTimer;

    // T�rol�k a tal�lt slotokhoz
    std::vector<int> m_potionSlots;
    std::vector<int> m_emptyHotbarSlots;

    // �j v�ltoz�k az inventory kezel�shez
    int m_inventoryOpenAttempts; // Inventory megnyit�si k�s�rletek sz�mol�sa
    std::chrono::steady_clock::time_point m_lastInventoryOpenAttempt; // Utols� megnyit�si k�s�rlet id�pontja

    // �j v�ltoz�k a folyamatos m�dhoz
    std::chrono::steady_clock::time_point m_lastContinuousRefillTime = std::chrono::steady_clock::now(); // Utols� folyamatos refill ind�t�s ideje
};
