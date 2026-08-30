#pragma once
#include <cstdint>

// Is an enemy actually visible to the local player right now?
//
// The old answer to this lived in sdk/trace.h and was always "Unknown", because
// there is no ray trace here. The aimbot then guarded its check with
// `trace::Available()`, which is false, so the whole visibility gate silently
// did nothing - the option was in the menu and had no effect at all.
//
// This uses what the game already computes instead of a trace. The server keeps
// a per-entity bitmask of which players currently see it (that is what feeds the
// radar and the spotted indicators) and replicates it to the client. Reading the
// local player's own bit in an enemy's mask is a real visibility answer, not an
// estimate.
//
// HONEST LIMITS, so nothing above this pretends otherwise:
//   - It is "spotted", which means in the viewer's field of view AND not
//     occluded. An enemy directly behind you is not spotted even with clear
//     line of sight. For gating an aimbot that is the right behaviour anyway,
//     since the target has to be in front to be aimed at.
//   - It is server-authoritative, so it lags by about one tick.
//   - It answers per entity, not per point, so it cannot tell a visible head
//     from an occluded foot on the same player.
namespace visibility {

// True once the spotted-state fields have been resolved from the schema. When
// false the caller must not silently treat everything as visible - say so.
bool Available();

// Local player's bit in the target's spotted mask.
bool IsVisible(uintptr_t targetPawn);

// Recomputes which bit belongs to the local player. Cheap; call once a frame.
void Update(uintptr_t entityList, uintptr_t localController);

// Which slot the local player occupies, or -1 while unknown.
int LocalSlot();

} // namespace visibility
