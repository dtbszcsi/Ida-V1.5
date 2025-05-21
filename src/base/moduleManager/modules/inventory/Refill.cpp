#include "refill.h"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <algorithm>
#include <chrono>
#include <vector>
#include <string>
#include <windows.h>

#include "menu/menu.h"
#include "moduleManager/commonData.h"
#include "util/minecraft/minecraft.h"
#include "util/keys.h"
#include "sdk/sdk.h"
#include "util/logger.h"
#include "configManager/settings.h"
#include "java/java.h"

void Refill::OnKeybind()
{
    if (!settings::RF_Enabled) { // Ha a modul ki van kapcsolva, a keybind bekapcsolja és elindítja a folyamatot
        settings::RF_Enabled = true;
        ResetRefill();             // 1. Reseteljük a belsõ állapotokat
    }
    else {
        if (SDK::minecraft && SDK::minecraft->IsInInventory() && (m_isRefilling || m_waitingForInventorySettle /* || m_shouldOpenInventory */)) {
            m_shouldCloseInventory = true;
        }
        else if (SDK::minecraft && SDK::minecraft->IsInInventory()) {
            // Akkor is zárjuk be, ha simán be volt kapcsolva a modul és nyitva az inventory
            m_shouldCloseInventory = true;
        }

        settings::RF_Enabled = false; // Elõbb kapcsoljuk ki a modult
        ResetRefill();             // Majd reseteljük az állapotokat
    }
}

// Új segédfüggvény az inventory megnyitásához (Ezt a funkciót magát nem töröljük, csak a hívásait)
void Refill::OpenInventory()
{
    Sleep(100); // Kicsit növeltük a Sleep-et

    INPUT input[2] = { 0 };

    // Key down
    input[0].type = INPUT_KEYBOARD;
    input[0].ki.wVk = 'E';
    input[0].ki.wScan = MapVirtualKey('E', MAPVK_VK_TO_VSC);
    input[0].ki.dwFlags = KEYEVENTF_SCANCODE;  // Scan kód használata

    // Key up
    input[1].type = INPUT_KEYBOARD;
    input[1].ki.wVk = 'E';
    input[1].ki.wScan = MapVirtualKey('E', MAPVK_VK_TO_VSC);
    input[1].ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;

    // Küldés egy lépésben
    SendInput(1, &input[0], sizeof(INPUT));
    Sleep(150);  // Hosszabb késleltetés
    SendInput(1, &input[1], sizeof(INPUT));

    // Módszer 3: DirectInput használata helyett keybd_event mint tartalék
    Sleep(150);
    keybd_event('E', MapVirtualKey('E', MAPVK_VK_TO_VSC), 0, 0);
    Sleep(100);
    keybd_event('E', MapVirtualKey('E', MAPVK_VK_TO_VSC), KEYEVENTF_KEYUP, 0);

}

// Új függvény a folyamatos módhoz: ellenõrzi, hogy indítani kell-e új refill ciklust
void Refill::CheckForContinuousRefill()
{
    // Ha nincs bekapcsolva a folyamatos mód, nem csinálunk semmit
    if (!settings::RF_ContinuousMode) {
        return;
    }

    // Ha az elõzõ refill óta nem telt el elég idõ, még várunk
    auto currentTime = std::chrono::steady_clock::now();
    if (std::chrono::duration_cast<std::chrono::milliseconds>(
        currentTime - m_lastContinuousRefillTime).count() < settings::RF_ContinuousDelay) {
        return;
    }

    if (SDK::minecraft && SDK::minecraft->IsInInventory() && !m_isRefilling /* && !m_shouldOpenInventory */
        && !m_waitingForInventorySettle && !m_shouldCloseInventory) {

        bool shouldCheckForRefill = true;

        // Hotbar ellenõrzés - ha nincs elég üres slot, ne indítson új ciklust
        if (shouldCheckForRefill && SDK::minecraft && SDK::minecraft->thePlayer) {
            CInventoryPlayer playerInventory = SDK::minecraft->thePlayer->GetInventory();
            if (playerInventory.GetInstance() != nullptr) {
                int emptySlots = 0;
                std::vector<CItemStack> mainInvStacks = playerInventory.GetMainInventory();
                // Ellenõrizzük, hogy mainInvStacks mérete legalább 9
                if (mainInvStacks.size() >= 9) {
                    for (int i = 0; i < 9; i++) {
                        if (mainInvStacks[i].GetInstance() == nullptr) {
                            emptySlots++;
                        }
                    }
                }
                else {
                    shouldCheckForRefill = false;
                }


                if (emptySlots == 0) {
                    shouldCheckForRefill = false; // Nincs üres slot a hotbaron
                }
            }
            else {
                shouldCheckForRefill = false; // Nincs érvényes playerInventory instance
            }
        }
        else {
            shouldCheckForRefill = false; // Nincs minecraft vagy player instance
        }


        if (shouldCheckForRefill) {
            ResetRefill();
            m_waitingForInventorySettle = true;
            m_inventorySettleTimer = std::chrono::steady_clock::now();
        }

        m_lastContinuousRefillTime = currentTime;
    }
}

void Refill::Update()
{
    // Elõször ellenõrizzük, hogy reagálnunk kell-e a keybindre közvetlenül
    static bool keyWasDown = false;
    bool keyIsDown = (settings::RF_Key != 0 && (GetAsyncKeyState(settings::RF_Key) & 0x8000));

    // Gomb lenyomás érzékelése
    if (keyIsDown && !keyWasDown) {
        OnKeybind(); // Meghívjuk a keybind funkciót, ha a gomb most lett lenyomva
    }
    keyWasDown = keyIsDown;

    // Folyamatos mód ellenõrzése - ha bekapcsolva, ellenõrizzük, kell-e újabb refill ciklust indítani
    CheckForContinuousRefill();

    // Ha a modul ki van kapcsolva
    if (!settings::RF_Enabled) {
        // Ha a modul ki van kapcsolva, de még be kellene zárni az inventory-t
        if (m_shouldCloseInventory) {
            if (SDK::minecraft && SDK::minecraft->IsInInventory()) {
                INPUT input[2] = {};
                // 'E' billentyû szimulálása a bezáráshoz
                input[0].type = INPUT_KEYBOARD; input[0].ki.wVk = 'E'; input[0].ki.dwFlags = 0; // Key down
                input[1].type = INPUT_KEYBOARD; input[1].ki.wVk = 'E'; input[1].ki.dwFlags = KEYEVENTF_KEYUP; // Key up
                SendInput(2, input, sizeof(INPUT));
            }
            m_shouldCloseInventory = false; // Feldolgoztuk
        }
        // Biztonsági reset, ha valamiért beragadt (m_shouldOpenInventory már nincs)
        if (m_isRefilling || m_waitingForInventorySettle) {
            ResetRefill();
        }
        return;
    }

    if (!SDK::minecraft || !SDK::minecraft->thePlayer || !SDK::minecraft->thePlayer->GetInstance()) {
        // Ha volt folyamatban refill, és elveszett a játékos/minecraft instance, resetelünk
        if (m_isRefilling || m_waitingForInventorySettle || m_shouldCloseInventory) {
            ResetRefill();
            m_shouldCloseInventory = false; // Ne próbáljon bezárni, ha nincs is minecraft instance
            settings::RF_Enabled = false; // Biztonsági kikapcsolás
        }
        return;
    }

    // Ha a modul engedélyezve van, de NINCS NYITVA az inventory, akkor lépjünk ki.
    // A refill folyamat CSAK AKKOR FUT, ha az inventory nyitva van.
    if (!SDK::minecraft->IsInInventory()) {
        // Ha volt folyamatban refill (m_isRefilling vagy m_waitingForInventorySettle) de bezáródott az inventory, resetelünk.
        if (m_isRefilling || m_waitingForInventorySettle) {
            ResetRefill(); // Ha épp refilleltünk/vártunk, reset
        }
        return; // Kilépünk, ha nincs nyitva az inventory
    }

    if (m_waitingForInventorySettle) {
        // Itt már tudjuk, hogy az inventory nyitva van, mert az elõzõ if (IsInInventory) checken átjutottunk.
        if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - m_inventorySettleTimer).count() >= 50) {
            m_waitingForInventorySettle = false;
        }
        else {
            return; // Még várunk a leülepedésre
        }
    }

    if (Menu::open) {
        // Ha a fõmenü nyitva van, és épp refilleltünk, resetelünk.
        if (m_isRefilling) ResetRefill();
        return;
    }

    if (!m_isRefilling)
    {
        FindPotions();
        FindEmptyHotbarSlots();

        if (m_potionSlots.empty() || m_emptyHotbarSlots.empty()) {
            ResetRefill();
            return; // Nincs mit refillezni, de a modul bekapcsolva maradhat.
        }

        m_isRefilling = true; // Indítjuk a refill folyamatot
        m_waitingForSecondClick_Refill = false;
        m_lastRefillTime = std::chrono::steady_clock::now();
        m_inventorySlotIndex = 0; // Biztos, ami biztos, index nullázása az elején
    }

    // 4. Aktív refill ciklus (kattintások végrehajtása)
    if (m_isRefilling)
    {
        // Ellenõrizzük, hogy van-e még teendõ (van-e még poti vagy üres slot)
        if (m_inventorySlotIndex >= m_potionSlots.size() || m_emptyHotbarSlots.empty()) {
            // Befejeztük a refilt vagy elfogytak az üres helyek a hotbaron
            ResetRefill();
            m_shouldCloseInventory = true; // Jelezzük, hogy be kell zárni az inventory-t a ciklus végén
            return;
        }

        // Kattintás késleltetés
        if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - m_lastRefillTime).count() < settings::RF_Delay) {
            return; // Még várunk a kattintásra
        }

        int windowId = 0; // Minecraft inventory ablak ID-ja
        // TODO: Ha külsõ konténert is kell kezelni, itt kell a windowId-t megfelelõen beállítani

        if (!SDK::minecraft->playerController) {
            // Ha elveszett a playerController, nem tudunk kattintani.
            ResetRefill();
            m_shouldCloseInventory = true; // Próbáljuk bezárni az inventory-t
            settings::RF_Enabled = false; // Kikapcsoljuk a modult
            return;
        }

        if (!m_waitingForSecondClick_Refill) {
            // ELSÕ KATTINTÁS: Potion kiválasztása az inventoryból
            // Fontos: m_potionSlots[m_inventorySlotIndex] a konténer beli slot ID
            SDK::minecraft->playerController->WindowClick(windowId, m_potionSlots[m_inventorySlotIndex], 0, 0, SDK::minecraft->thePlayer);
            m_lastRefillTime = std::chrono::steady_clock::now();
            m_waitingForSecondClick_Refill = true;
            // Ne növeljük itt az inventorySlotIndex-et, az a második kattintás után jön.
        }
        else {
            if (!m_emptyHotbarSlots.empty()) {
                SDK::minecraft->playerController->WindowClick(windowId, m_emptyHotbarSlots[0], 0, 0, SDK::minecraft->thePlayer);
                m_lastRefillTime = std::chrono::steady_clock::now();
                m_emptyHotbarSlots.erase(m_emptyHotbarSlots.begin());
                m_inventorySlotIndex++;
                m_waitingForSecondClick_Refill = false;

                // A ciklus végét itt is ellenõrizzük, ha elfogytak az üres helyek.
                if (m_emptyHotbarSlots.empty() || m_inventorySlotIndex >= m_potionSlots.size()) {
                    ResetRefill();
                    m_shouldCloseInventory = true;
                }
            }
            else {
                ResetRefill();
                m_shouldCloseInventory = true;
            }
        }
    }

    // 5. Inventory bezárása, ha jelölve van ÉS a refill befejezõdött (m_isRefilling == false)
    if (m_shouldCloseInventory && !m_isRefilling) {
        if (SDK::minecraft && SDK::minecraft->IsInInventory()) {
            INPUT input[2] = {};
            // 'E' billentyû szimulálása a bezáráshoz
            input[0].type = INPUT_KEYBOARD; input[0].ki.wVk = 'E'; input[0].ki.dwFlags = 0; // Key down
            input[1].type = INPUT_KEYBOARD; input[1].ki.wVk = 'E'; input[1].ki.dwFlags = KEYEVENTF_KEYUP; // Key up
            SendInput(2, input, sizeof(INPUT));
        }
        m_shouldCloseInventory = false; // Feldolgoztuk a bezárási kérést

        // Csak akkor kapcsoljuk ki a modult, ha NEM folyamatos módban vagyunk
        // Folyamatos módban a modul bekapcsolva marad, és várja, hogy újra kinyíljon az inventory.
        if (!settings::RF_ContinuousMode) {
            settings::RF_Enabled = false; // Kikapcsoljuk a modult a teljes ciklus után
        }
    }
}


void Refill::FindPotions()
{
    m_potionSlots.clear();
    if (!SDK::minecraft || !SDK::minecraft->thePlayer || !SDK::minecraft->thePlayer->GetInstance()) {
        return;
    }

    CInventoryPlayer playerInventory = SDK::minecraft->thePlayer->GetInventory();
    if (playerInventory.GetInstance() == nullptr) {
        return;
    }

    std::vector<CItemStack> mainInvStacks = playerInventory.GetMainInventory();
    if (mainInvStacks.empty()) return;

    const int mainInventoryStartSlot = 9;  // A main inventory (nem hotbar) kezdõ slotja a konténerben
    const int mainInventoryEndSlot = 36;   // A main inventory vége (slot 35-ig)

    for (int containerSlotId = mainInventoryStartSlot; containerSlotId < mainInventoryEndSlot; containerSlotId++)
    {
        int mainInvIndex = containerSlotId; // Ha a mainInvStacks a teljes 0-35-öt tartalmazza

        if (mainInvIndex < 9 || static_cast<size_t>(mainInvIndex) >= mainInvStacks.size()) { // Csak a 9-35 közötti részeket nézzük
            continue;
        }

        CItemStack itemStack = mainInvStacks[mainInvIndex];

        if (itemStack.GetInstance() == nullptr) continue;

        CItem item = itemStack.GetItem();
        if (item.GetInstance() == nullptr) continue;

        int itemId = item.GetID();
        int metadata = itemStack.GetMetadata();

        if ((itemId == settings::RF_PotionID || itemId == 373) &&
            (settings::RF_PotionMetadata == -1 || metadata == settings::RF_PotionMetadata || metadata == 16421))
        {
            m_potionSlots.push_back(containerSlotId); // A konténerbeli slot ID-t mentjük
        }
    }
}

void Refill::FindEmptyHotbarSlots()
{
    m_emptyHotbarSlots.clear();
    if (!SDK::minecraft || !SDK::minecraft->thePlayer || !SDK::minecraft->thePlayer->GetInstance()) {
        return;
    }

    CInventoryPlayer playerInventory = SDK::minecraft->thePlayer->GetInventory();
    if (playerInventory.GetInstance() == nullptr) {
        return;
    }

    std::vector<CItemStack> mainInvStacks = playerInventory.GetMainInventory();
    if (mainInvStacks.empty()) return;

    // A hotbar slotok a mainInvStacks vektorban a 0-8 indexeken vannak.
    // Ezek a konténerben a 36-44 slotoknak felelnek meg.
    for (int localHotbarIndex = 0; localHotbarIndex < 9; localHotbarIndex++)
    {
        if (static_cast<size_t>(localHotbarIndex) >= mainInvStacks.size()) { // Biztonsági ellenõrzés
            break;
        }

        CItemStack itemStack = mainInvStacks[localHotbarIndex];

        if (itemStack.GetInstance() == nullptr)
        {
            m_emptyHotbarSlots.push_back(36 + localHotbarIndex); // Konténerbeli slot ID mentése
        }
    }
}

void Refill::ResetRefill()
{
    m_isRefilling = false;
    m_inventorySlotIndex = 0;
    m_potionSlots.clear();
    m_emptyHotbarSlots.clear();
    m_waitingForSecondClick_Refill = false;
    // *** KISZEDVE: Automata nyitáshoz kapcsolódó állapotok ***
    // m_shouldOpenInventory = false;
    // m_inventoryOpenAttempts = 0;
    // m_lastInventoryOpenAttempt = std::chrono::steady_clock::now();
    // *** MEGMARADT: Bezáráshoz kapcsolódó állapot ***
    m_shouldCloseInventory = false;
    m_waitingForInventorySettle = false; // Ez marad, mert kell a leülepedés nyitás UTÁN
}

void Refill::RenderMenu()
{
    // A menü renderelés kódját általában nem kell módosítani az automata nyitás eltávolításához,
    // hacsak nem volt specifikus UI elem az automata nyitáshoz.
    // Feltételezzük, hogy az RF_Enabled kapcsoló most azt jelenti, hogy "engedélyezve van a refill, ha nyitva az inventory".

    Menu::ToggleWithKeybind(&settings::RF_Enabled, settings::RF_Key);

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10.f);
    ImGui::Separator();
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10.f);

    Menu::Slider("Delay between actions (ms)", &settings::RF_Delay, 0, 250);

    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Delay in milliseconds between picking up\nand placing a potion during refill.");
    }

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10.f);
    Menu::Checkbox("Continuous Mode", &settings::RF_ContinuousMode);

    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("If enabled, the refill module will automatically\ncheck if refill is needed when inventory is open.");
    }

    if (settings::RF_ContinuousMode) {
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5.f);
        Menu::Slider("Continuous mode delay (ms)", &settings::RF_ContinuousDelay, 0, 250);

        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Delay in milliseconds between automatic\nchecks in continuous mode (when inventory is open).");
        }
    }

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10.f);
    // Frissítjük a szöveget, hogy jobban illeszkedjen az új mûködéshez
    ImGui::TextWrapped("When your inventory is open and the module is enabled (either by keybind or continuous mode),\nit will attempt to move potions to your hotbar. It will close the inventory when finished.");

    if (settings::RF_ContinuousMode) {
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5.f);
        ImGui::TextWrapped("Continuous mode is enabled. The module will automatically check and refill when inventory is open.");
    }
}

// Valószínûleg a refill.h fájlból el kell távolítani a következõ tagváltozókat:
// bool m_shouldOpenInventory;
// int m_inventoryOpenAttempts;
// std::chrono::steady_clock::time_point m_lastInventoryOpenAttempt;