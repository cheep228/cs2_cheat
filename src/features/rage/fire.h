#pragma once
#include "core/globals.h"
#include "sdk/math.h"
#include "sdk/weapon.h"

// Actuation: everything that writes to the game or the input queue.
//
// The scanner never calls any of this. Keeping the decision ("this point is
// worth shooting") apart from the action ("click now") is what makes it
// possible to run the whole pipeline with Auto Fire off and still see, in the
// debug panel, exactly what it would have done.
namespace rage {
namespace fire {

// Speed below which the current weapon can actually hit what it is aimed at.
// Derived from the weapon class rather than a slider: a sniper needs to be
// standing still, a pistol barely cares, and asking the user to tune that per
// group is asking them to rediscover the game's own accuracy curve.
float StopSpeedFor(const weapon::Active& gun);

// Auto-stop. Returns true when movement is no longer holding the shot back.
// Only call this once a point has been chosen - a gate that runs every frame
// is just a movement blocker.
//   AutoStopOff    : always true, nothing is touched.
//   AutoStopNormal : true once the shooter is slow enough.
//   AutoStopFull   : same, and releases W/A/S/D while waiting.
bool AutoStop(int mode, const weapon::Active& gun, float speed);

// True when the weapon supports a scope and is not scoped yet. Weapons without
// one are never asked to scope.
bool NeedsScope(const weapon::Active& gun);

// One right-click, rate limited so a scope already coming up is not cancelled.
void ToggleScope();

// One left-click, rate limited to the weapon's own firing cycle. Returns false
// when the limiter swallowed it.
bool Shoot(const weapon::Active& gun);

// Puts the view on the chosen angles.
void WriteViewAngles(const Angle& angles);

// Drops the internal timers; call when the ragebot goes idle.
void Reset();

} // namespace fire
} // namespace rage
