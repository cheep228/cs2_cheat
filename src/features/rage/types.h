#pragma once
#include <cstdint>

#include "sdk/math.h"
#include "sdk/weapon.h"

// Shared vocabulary for the ragebot pipeline. Every stage takes and returns
// these types, which is what keeps Ragebot::Run readable instead of turning
// into one long function.
namespace rage {

// One candidate impact point on one hitbox of one enemy.
struct AimPoint {
    Vector3 position{};
    Angle   angle{};            // view angles that put the crosshair on it

    int   hitbox   = -1;        // cfg::RageHitbox
    int   bone     = -1;
    bool  isHead   = false;
    bool  isLimb   = false;
    bool  isCentre = false;     // the hitbox centre sample, not an edge one

    float distance  = 0.f;
    float fov       = 0.f;      // angular distance from the current view
    float damage    = 0.f;      // after falloff, armour and the wall model
    float freeAir   = 0.f;      // same shot with no walls assumed
    float hitchance = 0.f;      // 0..100

    bool  lethal = false;

    // No trace backend exists (see sdk/trace.h), so `visible` is only
    // meaningful when `visibilityKnown` is true. The scorer must not treat an
    // unknown point as confirmed visible.
    bool  visible         = true;
    bool  visibilityKnown = false;

    float score = 0.f;
};

// An enemy that survived the cheap filters and is worth scanning.
struct Target {
    int       slot   = -1;
    uintptr_t pawn   = 0;
    int       health = 0;
    int       team   = 0;
    Vector3   origin{};
    float     distance = 0.f;
    float     fov      = 0.f;   // to the nearest enabled bone, before scanning

    weapon::Armor armor{};
    char          name[32]{};
};

// A target together with the best point found on it.
struct Solution {
    bool     valid = false;
    Target   target{};
    AimPoint point{};

    int pointsTested = 0;
    int pointsPassed = 0;

    // Best damage seen on this target even among rejected points, so the debug
    // panel can say "closest we got was 34 of 50" instead of just "rejected".
    float bestDamageSeen    = 0.f;
    float bestHitchanceSeen = 0.f;
};

// Why the pipeline produced nothing. Ordered from earliest gate to latest so
// the panel can report the first thing that actually blocked the shot.
enum class Stage {
    Idle = 0,
    Disabled,
    KeyUp,
    NoLocalPawn,
    NoBones,
    NoWeapon,
    WeaponGroupOff,
    NoHitbox,
    NoCandidates,
    NoneInFov,
    NoBonePositions,
    BelowDamage,
    BelowHitchance,
    AutoStopWaiting,
    Firing
};

const char* StageName(Stage s);

// Everything the debug panel needs to explain a decision, filled every frame.
struct Diagnostics {
    Stage stage = Stage::Idle;

    const char* weapon      = "none";
    const char* group       = "-";
    bool        groupActive = false;

    int candidates   = 0;   // enemies alive, not dormant, not teammates
    int inFov        = 0;   // of those, inside the FOV limit
    int pointsTested = 0;
    int pointsPassed = 0;

    bool hasTarget = false;
    char targetName[32] = "-";
    int  targetSlot     = -1;
    int  targetHealth   = 0;

    int   hitbox = -1;
    float damage = 0.f;
    int   requiredDamage = 0;
    float hitchance = 0.f;
    int   requiredHitchance = 0;
    float fov    = 0.f;
    float maxFov = 0.f;

    bool lethal          = false;
    bool overrideHeld    = false;
    bool forceBodyHeld   = false;
    bool visibilityKnown = false;

    const char* result = "IDLE";
};

} // namespace rage
