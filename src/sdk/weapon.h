#pragma once
#include <cstdint>

namespace weapon {

// Broad behaviour classes. The damage model only needs this much: the exact
// per-weapon spread tables live in the game's own scripts, which are not
// reachable from a read-only overlay.
enum class Kind : int {
    Unknown = 0,
    Pistol,
    Smg,
    Rifle,
    Sniper,
    Shotgun,
    Machinegun,
    Knife,
};

// Ballistics taken from the published CS2 weapon values. Damage, armour ratio
// and the range falloff are exact; the inaccuracy figure is a per-class
// approximation used only by the hitchance estimate.
struct Info {
    const char* name        = "unknown";
    Kind        kind        = Kind::Unknown;
    float       damage      = 0.f;
    float       armorRatio  = 0.5f;   // already halved, ready to multiply
    float       penetration = 1.f;    // weapon penetration modifier
    float       rangeModifier = 1.f;  // damage multiplier per 500 units
    float       maxRange    = 8192.f;
    float       headshotMul = 4.f;
    float       baseSpread  = 0.f;    // cone at rest, tangent units
    bool        scopeable   = false;
};

// Item definition index -> ballistics. Never null; unknown ids come back as a
// zero-damage Unknown entry.
const Info& ById(int itemDefIndex);

// What the local player is holding right now.
struct Active {
    bool      ok        = false;
    uintptr_t entity    = 0;
    int       defIndex  = 0;
    const Info* info    = nullptr;
    float     accuracyPenalty = 0.f;   // live, grows while firing
    bool      reloading = false;
    bool      scoped    = false;

    // Readiness, for the trigger bot. nextAttackTick is a game tick, so it is
    // compared against the global vars tick counter rather than a wall clock.
    int       clip           = -1;   // -1 when the field did not read
    int       nextAttackTick = 0;
};

Active Local(uintptr_t localPawn);

// Armour state of a target pawn, for the damage model.
struct Armor {
    int  value  = 0;
    bool helmet = false;
};

Armor ArmorOf(uintptr_t pawn);

} // namespace weapon
