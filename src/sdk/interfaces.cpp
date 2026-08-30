#include <Windows.h>

#include <cmath>

#include "core/log.h"
#include "sdk/entity.h"
#include "sdk/interfaces.h"
#include "sdk/modules.h"

namespace ifaces {

Set set;

namespace {

using CreateInterfaceFn = void*(*)(const char*, int*);

// Asks one module for a named interface. A miss is a warning rather than an
// error: several of these are optional, and the caller decides how much it
// cares.
uintptr_t FromModule(const char* moduleName, const char* version) {
    const HMODULE mod = GetModuleHandleA(moduleName);
    if (!mod) {
        DBG_W("iface", "%-26s : %s not loaded", version, moduleName);
        return 0;
    }

    const auto create =
        reinterpret_cast<CreateInterfaceFn>(GetProcAddress(mod, "CreateInterface"));
    if (!create) {
        DBG_E("iface", "%-26s : %s exports no CreateInterface", version, moduleName);
        return 0;
    }

    const auto ptr = reinterpret_cast<uintptr_t>(create(version, nullptr));
    if (!mem::Valid(ptr)) {
        DBG_E("iface", "%-26s : NOT FOUND in %s", version, moduleName);
        return 0;
    }

    DBG_OK("iface", "%-26s @ 0x%llX", version, static_cast<unsigned long long>(ptr));
    return ptr;
}

// CCSGOInput is not a global pointer but the return value of a getter. The
// pattern lands on the CALL, so the target is resolved and then invoked.
uintptr_t ResolveCsgoInput() {
    const uintptr_t at = modules::Find(
        modules::set.client,
        "E8 ? ? ? ? 48 8B 93 ? ? ? ? 48 8B C8 E8 ? ? ? ? 48 89 BB ? ? ? ? 8B 83",
        "CCSGOInput getter");
    if (!at)
        return 0;

    const uintptr_t fn = sig::RipTarget(at, 1, 5);
    if (!mem::Valid(fn)) {
        DBG_E("iface", "CCSGOInput: getter address is not sane");
        return 0;
    }

    // Calling into the game this early is the one unavoidable risk here; the
    // address was pattern-verified, and a bad call would have shown up as a
    // failed pattern above rather than as a wrong-but-plausible pointer.
    const auto getter = reinterpret_cast<uintptr_t(__fastcall*)()>(fn);
    const uintptr_t input = getter();

    if (!mem::Valid(input)) {
        DBG_E("iface", "CCSGOInput: getter returned nothing");
        return 0;
    }

    DBG_OK("iface", "%-26s @ 0x%llX", "CCSGOInput",
           static_cast<unsigned long long>(input));
    return input;
}

// A wrong global-vars offset silently corrupts every timing decision built on
// it, so the values are checked against what CS2 can actually produce before
// the pointer is trusted.
bool GlobalVarsLookSane(const GlobalVars* g) {
    if (!g)
        return false;

    if (g->maxClients < 1 || g->maxClients > 65) {
        DBG_W("iface", "global vars rejected: maxClients = %d", g->maxClients);
        return false;
    }

    // 64 tick is 0.015625; 128 tick is 0.0078125. Anything outside this band
    // is not a tick interval.
    if (!(g->intervalPerTick > 0.001f && g->intervalPerTick < 0.1f)) {
        DBG_W("iface", "global vars rejected: intervalPerTick = %f", g->intervalPerTick);
        return false;
    }

    if (!(g->curTime >= 0.f) || !std::isfinite(g->curTime)) {
        DBG_W("iface", "global vars rejected: curTime = %f", g->curTime);
        return false;
    }

    return true;
}

} // namespace

const GlobalVars* Globals() {
    if (!set.globalVarsTrusted || !mem::Valid(set.globalVars))
        return nullptr;
    return reinterpret_cast<const GlobalVars*>(set.globalVars);
}

bool ReadyForHooks() {
    return set.csgoInput != 0 && set.source2Client != 0;
}

bool Resolve() {
    modules::Refresh();
    if (!modules::Ready()) {
        DBG_W("iface", "client.dll / engine2.dll not loaded yet - retrying later");
        return false;
    }

    if (!set.schemaSystem)
        set.schemaSystem = FromModule("schemasystem.dll", "SchemaSystem_001");
    if (!set.engineClient)
        set.engineClient = FromModule("engine2.dll", "Source2EngineToClient001");
    if (!set.source2Client)
        set.source2Client = FromModule("client.dll", "Source2Client002");
    if (!set.engineCvar)
        set.engineCvar = FromModule("tier0.dll", "VEngineCvar007");
    if (!set.inputSystem)
        set.inputSystem = FromModule("inputsystem.dll", "InputSystemVersion001");
    if (!set.sceneSystem)
        set.sceneSystem = FromModule("scenesystem.dll", "SceneSystem_002");
    if (!set.materialSystem)
        set.materialSystem = FromModule("materialsystem2.dll", "VMaterialSystem2_001");

    if (!set.csgoInput)
        set.csgoInput = ResolveCsgoInput();

    if (!set.entitySystem) {
        set.entitySystem = modules::FindGlobalPtr(
            modules::set.client,
            "48 8B 0D ? ? ? ? 48 8B D3 E8 ? ? ? ? FF C5", 3, 7,
            "CGameEntitySystem");
    }

    if (!set.globalVars) {
        set.globalVars = modules::FindGlobalPtr(
            modules::set.client,
            "48 89 15 ? ? ? ? 48 89 42", 3, 7,
            "CGlobalVarsBase");

        if (set.globalVars) {
            const auto* g = reinterpret_cast<const GlobalVars*>(set.globalVars);
            set.globalVarsTrusted = GlobalVarsLookSane(g);
            if (set.globalVarsTrusted) {
                DBG_OK("iface", "global vars trusted: %d tick, maxClients %d",
                       static_cast<int>(1.f / g->intervalPerTick), g->maxClients);
            } else {
                DBG_E("iface", "global vars found but implausible - not trusted");
            }
        }
    }

    const bool ready = ReadyForHooks();
    if (!ready)
        DBG_W("iface", "not ready for game hooks: csgoInput=%s source2Client=%s",
              set.csgoInput ? "ok" : "MISSING",
              set.source2Client ? "ok" : "MISSING");

    return ready;
}

} // namespace ifaces
