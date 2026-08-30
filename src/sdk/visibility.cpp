#include <cstdint>

#include "core/log.h"
#include "sdk/entity.h"
#include "sdk/entitylist.h"
#include "sdk/offsets.h"
#include "sdk/resolve.h"
#include "sdk/visibility.h"

namespace visibility {
namespace {

constexpr int kMaxPlayers = 64;

int  g_localSlot = -1;
bool g_reported  = false;

} // namespace

bool Available() {
    // Both fields have baked fallbacks, so "available" means the schema either
    // confirmed them or the fallbacks are at least non-zero. A zero offset here
    // would read the wrong member and hand back nonsense.
    return schema::m_entitySpottedState > 0 && schema::m_bSpottedByMask >= 0 &&
           g_localSlot >= 0;
}

int LocalSlot() {
    return g_localSlot;
}

void Update(uintptr_t entityList, uintptr_t localController) {
    if (!mem::Valid(entityList) || !mem::Valid(localController)) {
        g_localSlot = -1;
        return;
    }

    // The bit index is the player's slot, so find which slot holds the local
    // controller. Cached until the pointer changes, which is a map change or a
    // reconnect.
    static uintptr_t lastController = 0;
    if (localController == lastController && g_localSlot >= 0)
        return;

    lastController = localController;
    g_localSlot = -1;

    for (int slot = 1; slot <= kMaxPlayers; ++slot) {
        const uintptr_t controller = entlist::Get(entityList, slot);
        if (controller == localController) {
            g_localSlot = slot - 1;   // masks are zero-based
            break;
        }
    }

    if (g_localSlot >= 0 && !g_reported) {
        g_reported = true;
        DBG_OK("visible", "spotted mask ready - local slot %d, state +0x%llX mask +0x%llX",
               g_localSlot,
               static_cast<unsigned long long>(schema::m_entitySpottedState),
               static_cast<unsigned long long>(schema::m_bSpottedByMask));
    }
}

bool IsVisible(uintptr_t targetPawn) {
    if (!Available() || !mem::Valid(targetPawn))
        return false;

    const uintptr_t state = targetPawn + schema::m_entitySpottedState +
                            schema::m_bSpottedByMask;

    // The mask is two dwords: slots 0..31 and 32..63.
    const int word = g_localSlot >= 32 ? 1 : 0;
    const int bit  = g_localSlot & 31;

    const uint32_t mask = mem::Read<uint32_t>(state + word * 4);
    return (mask & (1u << bit)) != 0;
}

} // namespace visibility
