// main.h
#pragma once
#include <Windows.h> // HMODULE-hez

// Lehet, hogy más include-ok vagy elõre deklarációk is vannak itt

struct Main // Vagy 'class Main'
{
    static HMODULE hModule; // A hModule STATIKUS TAGVÁLTOZÓ DEKLARÁCIÓJA

    static void Init();
    static void Shutdown();

    // ... egyéb Main specifikus deklarációk ...
};