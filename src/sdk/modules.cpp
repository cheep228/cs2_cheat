#include <Windows.h>

#include "core/log.h"
#include "sdk/entity.h"
#include "sdk/modules.h"

namespace modules {

Set set;

namespace {

// Resolves one module and logs the outcome exactly once per state change, so a
// module that appears late does not spam the console while we wait for it.
void Resolve(sig::Region& out, const char* name) {
    if (out.base)
        return;   // already resolved; modules are not unloaded mid-session

    out = sig::Executable(name);
    if (out.base) {
        DBG_OK("module", "%-22s base 0x%llX  .text 0x%llX (%zu KB)", name,
               static_cast<unsigned long long>(out.base),
               static_cast<unsigned long long>(out.start), out.size / 1024);
    }
}

} // namespace

void Refresh() {
    Resolve(set.client,          "client.dll");
    Resolve(set.engine2,         "engine2.dll");
    Resolve(set.schemasystem,    "schemasystem.dll");
    Resolve(set.scenesystem,     "scenesystem.dll");
    Resolve(set.rendersystem,    "rendersystemdx11.dll");
    Resolve(set.materialsystem2, "materialsystem2.dll");
    Resolve(set.vphysics2,       "vphysics2.dll");
    Resolve(set.inputsystem,     "inputsystem.dll");
    Resolve(set.particles,       "particles.dll");
    Resolve(set.tier0,           "tier0.dll");
}

bool Ready() {
    return set.client.base != 0 && set.engine2.base != 0;
}

uintptr_t Find(const sig::Region& region, const char* pattern, const char* what) {
    if (!region.start) {
        DBG_W("sig", "%s: module not loaded", what);
        return 0;
    }

    const uintptr_t at = sig::Find(region, pattern);
    if (!at) {
        // A missing signature is the single most likely consequence of a game
        // update, so it is reported loudly and by name rather than swallowed.
        DBG_E("sig", "%s: PATTERN NOT FOUND - this feature stays off", what);
        return 0;
    }

    DBG_I("sig", "%-26s @ 0x%llX (+0x%llX)", what,
          static_cast<unsigned long long>(at),
          static_cast<unsigned long long>(at - region.base));
    return at;
}

uintptr_t FindGlobalPtr(const sig::Region& region, const char* pattern,
                        int relOffset, int instrLen, const char* what) {
    const uintptr_t at = Find(region, pattern, what);
    if (!at)
        return 0;

    const uintptr_t slot = sig::RipTarget(at, relOffset, instrLen);
    if (!mem::Valid(slot)) {
        DBG_E("sig", "%s: rip target 0x%llX is not a sane address", what,
              static_cast<unsigned long long>(slot));
        return 0;
    }

    const uintptr_t value = mem::Read<uintptr_t>(slot);
    if (!mem::Valid(value)) {
        // The slot exists but is still null: the game has not created that
        // singleton yet. Not an error, just not ready.
        DBG_W("sig", "%s: slot @0x%llX is empty (game not ready?)", what,
              static_cast<unsigned long long>(slot));
        return 0;
    }

    DBG_OK("sig", "%-26s -> 0x%llX", what, static_cast<unsigned long long>(value));
    return value;
}

} // namespace modules
