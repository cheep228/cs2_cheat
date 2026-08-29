#include <Windows.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "core/log.h"
#include "sdk/entity.h"
#include "sdk/gameinfo.h"
#include "sdk/offsets.h"
#include "sdk/sigscan.h"

namespace {

// Strips `levels` trailing path components in place.
void TrimComponents(wchar_t* path, int levels) {
    for (int i = 0; i < levels; ++i) {
        wchar_t* slash = wcsrchr(path, L'\\');
        if (!slash)
            return;
        *slash = L'\0';
    }
}

int ParseSteamInf(const wchar_t* path) {
    FILE* f = nullptr;
    if (_wfopen_s(&f, path, L"r") != 0 || !f)
        return 0;

    char line[256];
    int build = 0;
    while (fgets(line, sizeof(line), f)) {
        // ClientVersion is the build number; PatchVersion is the marketing one.
        if (strncmp(line, "ClientVersion=", 14) == 0) {
            build = atoi(line + 14);
            break;
        }
    }
    fclose(f);
    return build;
}

} // namespace

namespace gameinfo {

int BuildNumber() {
    static int cached = -1;
    if (cached >= 0)
        return cached;

    cached = 0;

    // engine2.dll: 89 05 rel32  mov [rip+dwBuildNumber], eax
    const sig::Region engine = sig::Executable("engine2.dll");
    if (!engine.base) {
        DBG_W("build", "engine2.dll not loaded - cannot read the build number");
        return cached;
    }

    const uintptr_t hit = sig::Find(engine, "89 05 ? ? ? ? 48 8D 0D ? ? ? ? FF 15");
    const uintptr_t addr = sig::RipTarget(hit, 2, 6);
    if (!addr) {
        DBG_W("build", "dwBuildNumber signature did not match");
        return cached;
    }

    cached = mem::Read<int>(addr);
    return cached;
}

int ClientVersion() {
    // The menu asks for this every frame; steam.inf cannot change while the
    // game runs, so read it once.
    static int cached = -1;
    if (cached >= 0)
        return cached;

    cached = 0;

    wchar_t exe[MAX_PATH]{};
    if (!GetModuleFileNameW(nullptr, exe, MAX_PATH))
        return cached;

    // ...\game\bin\win64\cs2.exe -> ...\game
    TrimComponents(exe, 3);

    wchar_t inf[MAX_PATH]{};
    _snwprintf_s(inf, MAX_PATH, _TRUNCATE, L"%s\\csgo\\steam.inf", exe);
    cached = ParseSteamInf(inf);

    if (cached == 0)
        DBG_W("build", "steam.inf not found at %ls\\csgo\\steam.inf", exe);
    return cached;
}

bool ReportBuildMatch() {
    DBG_I("build", "steam client version %d (informational)", ClientVersion());

    const int game = BuildNumber();

    if (game == 0) {
        DBG_W("build", "engine build number unavailable; baked offsets are for %d",
              offsets::kDumpBuild);
        return false;
    }

    if (game == offsets::kDumpBuild) {
        DBG_OK("build", "engine build %d == baked dump %d", game, offsets::kDumpBuild);
        return true;
    }

    // Not fatal any more: resolve::Run() reads the live values regardless.
    DBG_W("build", "engine build %d differs from the baked dump %d - "
                   "runtime resolution will correct the offsets",
          game, offsets::kDumpBuild);
    return false;
}

} // namespace gameinfo
