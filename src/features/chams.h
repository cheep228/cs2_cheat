#pragma once
#include <cstdint>

#include "core/globals.h"

// Glow and "chams" both drive CGlowProperty on the pawn — the game's own glow
// system. That makes them a memory WRITE, unlike everything else here.
//
// Honest scope note: only ChamsGlow is a plain glow. Metallic / Plastic / Latex
// are different glow types and intensities, not real material overrides — those
// would need a hook in the model render path.
namespace chams {

// Applies the group's glow/chams settings to one pawn. Call every frame while
// the entity is visible; the game resets the property on respawn.
void Apply(uintptr_t pawn, const cfg::EspGroup& group);

// Turns the glow back off for a pawn we no longer highlight.
void Clear(uintptr_t pawn);

// Number of pawns written to this frame, for the diagnostics panel.
extern int lastApplied;

} // namespace chams
