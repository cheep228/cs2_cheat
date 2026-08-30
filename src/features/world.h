#pragma once
#include <cstdint>

#include "imgui.h"
#include "sdk/math.h"

// Local-only world visuals.
//
// Both features here are visual suppression, not game logic: nothing is
// removed from the entity system and no grenade is destroyed. Anti-smoke
// rewrites the smoke projectile's own render-start tick, which is the input
// the volumetric smoke pass reads; anti-flash clamps the local pawn's flash
// overlay. Each remembers what it overwrote and puts it back when its option
// goes off, so turning the setting off really does restore stock behaviour.
namespace world {

// The C4, in whichever of its two states it currently exists.
//
// These are different entities, not two flags on one: a dropped or carried
// bomb is a C_C4 (the weapon), a planted one is a C_PlantedC4. Both are looked
// up by the entity's own designer name through the schema, never by model
// string or by guessing from coordinates.
struct Bomb {
    // --- dropped weapon_c4 lying in the world -------------------------------
    bool      droppedFound  = false;
    uintptr_t droppedEntity = 0;
    Vector3   droppedOrigin{};
    float     droppedMeters = 0.f;

    // --- planted_c4 ---------------------------------------------------------
    bool      found    = false;   // a planted bomb exists
    uintptr_t entity   = 0;
    Vector3   origin{};
    bool      ticking  = false;
    bool      defused  = false;
    int       site     = -1;      // 0 = A, 1 = B, -1 unknown

    // Seconds left before detonation, from the bomb's own blow time minus the
    // game's current time. Never a wall clock and never counted from when the
    // entity was first seen - both drift.
    bool  timerValid = false;
    float remaining  = 0.f;

    // Defuse, same rule: an absolute game time, not a stopwatch started when
    // the defuse became visible.
    bool  beingDefused     = false;
    bool  defuseValid      = false;
    float defuseRemaining  = 0.f;

    // True when the defuse finishes before the bomb does.
    bool  defuseInTime = false;

    // Set once per state change so the console explains a blank ESP.
    float lastCurTime = 0.f;
    float lastBlowTime = 0.f;
    bool  w2sOk = false;
};

extern Bomb bomb;

// Per-frame diagnostics for the Visuals > World panel.
struct Stats {
    int   smokesTracked    = 0;   // smoke projectiles currently in the cache
    int   smokesSuppressed = 0;   // of those, written to this frame
    bool  localPawnOk      = false;
    bool  flashClamped     = false;
    float flashOverlay     = 0.f; // live m_flFlashOverlayAlpha, for the panel
};

extern Stats stats;

// Call once per frame from the present hook, before the ESP draws.
void Run();

// Draws the bomb marker and, when enabled and valid, the countdown. Called
// with the same background draw list the ESP uses.
void Draw(ImDrawList* draw);

// Puts every value this module overwrote back and forgets the cache. Called
// on unload so the game is not left with smoke suppressed.
void Restore();

} // namespace world
