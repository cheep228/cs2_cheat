#pragma once
#include <cstddef>
#include <cstdint>

// The entity list is a two-level table whose exact shape drifts between builds:
// the identity-chunk array can move inside the entity system, and the per-entry
// stride follows sizeof(CEntityIdentity).
//
// Rather than hardcode a guess, this calibrates against a known-good anchor:
// dwLocalPlayerController points at a real controller that MUST be reachable
// through the table, so the layout that produces it is the correct one.
namespace entlist {

struct Layout {
    ptrdiff_t chunkArray = 0x10;   // identity-chunk array inside the entity system
    ptrdiff_t stride     = 0x78;   // bytes per CEntityIdentity
    ptrdiff_t instance   = 0x00;   // m_pInstance inside the identity
    bool      calibrated = false;
};

extern Layout layout;

// Tries every plausible layout until the local controller falls out of it.
// Needs to run while in a match. Returns true once calibrated (and then
// becomes a no-op).
bool Calibrate(uintptr_t entityList, uintptr_t localController, uintptr_t localPawn);

// Resolves a slot with the current layout.
uintptr_t Get(uintptr_t entityList, int index);

} // namespace entlist
