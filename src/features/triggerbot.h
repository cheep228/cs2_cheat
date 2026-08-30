#pragma once
#include <cstdint>

#include "sdk/usercmd.h"

namespace triggerbot {

// Hit groups the trace can report, in the order they appear in the menu.
enum HitGroup : int {
    GroupHead = 0,
    GroupChest,
    GroupStomach,
    GroupArms,
    GroupLegs,
    GroupCount
};

const char* GroupName(int group);

// Why a shot was not taken. Reported in the panel so a silent trigger bot can
// be diagnosed instead of guessed at.
enum class Reject {
    None = 0,
    Disabled,
    KeyUp,
    LocalDead,
    NoWeapon,        // knife, grenade, C4 - nothing the trigger fires
    WeaponNotReady,  // reloading, empty, or next attack tick not reached
    NoTarget,        // the ray hit nobody
    Teammate,
    InvalidHitgroup,
    NotVisible,
    Delay,           // target held for less than the configured time
    Fired
};

const char* RejectName(Reject r);

struct Stats {
    bool   enabled     = false;
    bool   keyHeld     = false;
    Reject reject      = Reject::Disabled;

    bool   targetFound = false;   // the ray hit a player pawn at all
    int    targetSlot  = -1;
    char   targetName[32] = "-";
    int    hitGroup    = -1;
    float  hitDistance = 0.f;     // along the ray, in units

    bool   teamValid   = false;
    bool   visible     = false;
    bool   visibilityKnown = false;
    bool   weaponReady = false;
    bool   delayElapsed = false;

    float  heldForMs   = 0.f;     // how long this target has been under the ray
    int    tickCount   = 0;       // proves this runs per command, not per frame
    int    nextAttackTick = 0;
    int    clip        = -1;
    const char* weapon = "none";

    uint64_t shotsFired = 0;
};

extern Stats stats;

// Called from the CreateMove hook. Everything happens here: the trace, the
// gates and the attack bit. Running per command rather than per frame is what
// makes the delay and the fire rate independent of the frame rate.
void Run(usercmd::UserCmd* cmd, uintptr_t localPawn);

} // namespace triggerbot
