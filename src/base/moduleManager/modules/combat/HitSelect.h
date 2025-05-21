#pragma once
#include "moduleManager/moduleBase.h"   // Az alap modul oszt�lyod
#include "configManager/settings.h"    // A settings::HS_... el�r�s�hez
#include "sdk/sdk.h"                   // CEntity �s m�s SDK elemek miatt (FONTOS!)
#include <string>
#include <chrono>                      // Az m_lastAttackTime miatt

class HitSelect : public ModuleBase {
public:
    // Konstruktor inline defin�ci�val
    HitSelect() : m_name("HitSelect"), m_category("Combat"), m_lastAttackTime(0) {}

    // ModuleBase virtu�lis f�ggv�nyeinek fel�l�r�sa
    void Update() override;
    void RenderOverlay() override { /* Maradhat �resen, ha nem rajzol overlay-t */ }
    void RenderHud() override { /* Maradhat �resen, ha nem rajzol HUD-ot */ }
    void RenderMenu() override;

    // Getterek JAV�TVA: 'const' elt�vol�tva, hogy megfeleljenek a ModuleBase-nek
    std::string GetName() override { return m_name; }
    std::string GetCategory() override { return m_category; }
    int GetKey() override { return settings::HS_Key; }
    bool IsEnabled() override { return settings::HS_Enabled; }

    void SetEnabled(bool enabled) override { settings::HS_Enabled = enabled; }
    void Toggle() override { settings::HS_Enabled = !settings::HS_Enabled; }

private:
    std::string m_name;
    std::string m_category;
    long long m_lastAttackTime; // T�pus long long-ra

    // Seg�df�ggv�nyek deklar�ci�i (ezek a HitSelect oszt�ly tagjai)
    float GetDistanceToEntity(CEntity& target);
    bool IsValidAttackableTarget(CEntity& target);
    void SimulateAttack();
};
