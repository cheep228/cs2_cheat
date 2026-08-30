#include <Windows.h>

#include "core/globals.h"
#include "core/hooks.h"
#include "core/log.h"
#include "core/game_hooks.h"
#include "features/world.h"
#include "core/settings.h"
#include "sdk/gameinfo.h"
#include "sdk/resolve.h"

namespace {

// client.dll is loaded lazily by the engine, so wait for it instead of
// assuming it is present the moment we are injected.
uintptr_t WaitForClientModule(int timeoutMs) {
    for (int waited = 0; waited < timeoutMs; waited += 100) {
        if (HMODULE m = GetModuleHandleW(L"client.dll"))
            return reinterpret_cast<uintptr_t>(m);

        if (waited && waited % 5000 == 0)
            DBG_T("core", "still waiting for client.dll (%d s)", waited / 1000);
        Sleep(100);
    }
    return 0;
}

void LogHostProcess() {
    wchar_t path[MAX_PATH]{};
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    DBG_I("core", "host process : %ls", path);
    DBG_I("core", "pid          : %lu", GetCurrentProcessId());
    DBG_I("core", "module base  : 0x%llX",
          static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(g::selfModule)));
}

[[noreturn]] void Die(const char* what) {
    DBG_E("core", "%s", what);
    DBG_E("core", "aborting - console stays open for 10 s");
    Sleep(10000);
    dbg::Close();
    FreeLibraryAndExitThread(g::selfModule, 1);
}

DWORD WINAPI MainThread(LPVOID) {
    dbg::Open(L"cs2_overlay - debug");
    LogHostProcess();

    // The loader hands the menu key over in an ini next to the extracted DLL,
    // so whatever was picked in its dropdown is what opens the menu in game.
    settings::Load();

    DBG_I("core", "waiting for client.dll ...");
    g::client = WaitForClientModule(60'000);
    if (!g::client)
        Die("client.dll never appeared after 60 s - wrong process?");

    DBG_OK("core", "client.dll   : 0x%llX",
           static_cast<unsigned long long>(g::client));

    // Cheapest possible sanity check: the game states its own build number, so
    // a mismatch with the dump explains every wrong read before we make one.
    gameinfo::ReportBuildMatch();

    // Ask the game itself for the real offsets. Whatever this resolves wins
    // over the baked defaults, so a build mismatch stops mattering.
    resolve::Run();

    DBG_I("core", "installing D3D11 hooks ...");
    if (!hooks::Init())
        Die("hook installation failed - see the messages above");

    DBG_OK("core", "ready - INSERT toggles the menu, END unloads");

    while (!cfg::c.unloadQueued && !g::unload)
        Sleep(50);

    DBG_W("core", "unload requested");
    world::Restore();
    gamehooks::Remove();
    hooks::Shutdown();
    Sleep(100);

    DBG_OK("core", "clean unload, goodbye");
    Sleep(1200);          // let the last lines be readable
    dbg::Close();

    FreeLibraryAndExitThread(g::selfModule, 0);
    return 0;  // not reached
}

} // namespace

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(module);
        g::selfModule = module;

        if (HANDLE t = CreateThread(nullptr, 0, MainThread, nullptr, 0, nullptr))
            CloseHandle(t);
    }
    return TRUE;
}
