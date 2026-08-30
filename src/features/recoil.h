#pragma once
#include <cstdint>

#include "sdk/math.h"
#include "sdk/usercmd.h"

// Recoil control.
//
// Compensates the systematic part of where a spraying weapon walks, using the
// punch the game itself reports for the local pawn. Nothing here is a recorded
// spray pattern and nothing is random - both would be guesses about a value
// the game already publishes.
//
// It deliberately does not try to flatten SPREAD. Recoil is the weapon walking
// off the aim point in a repeatable way; spread is the per-bullet scatter
// inside the cone. Full strength cancels the first and leaves the second, so
// "100%" is not a promise of a motionless weapon.
namespace recoil {

struct Stats {
    bool  enabled   = false;
    bool  running   = false;   // actually correcting this command
    int   shotsFired = 0;

    Vector3 currentPunch{};
    Vector3 previousPunch{};
    Vector3 deltaPunch{};

    float appliedPitch = 0.f;
    float appliedYaw   = 0.f;

    // Correction still owed, when smoothing spreads it over commands.
    float pendingPitch = 0.f;
    float pendingYaw   = 0.f;

    const char* weapon = "none";
    const char* reason = "idle";
    int   tickCount = 0;
};

extern Stats stats;

// Called from the CreateMove hook, after the aimbot has had its say and before
// the trigger traces, so there is exactly one final angle per command.
void Run(usercmd::UserCmd* cmd, uintptr_t localPawn);

} // namespace recoil
