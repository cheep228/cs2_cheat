#pragma once
#include <cstdint>

#include "sdk/math.h"
#include "sdk/weapon.h"

// Damage estimation for the ragebot's minimum-damage gate.
//
// HONEST LIMIT: this overlay is read-only and has no ray trace, so nothing here
// can see world geometry. Free-air damage (range falloff, armour, helmet,
// headshot multiplier) is exact; "penetration" is modelled as the damage a shot
// would still carry after a configured number of walls, which makes the gate
// conservative rather than clairvoyant. It never claims a wall is there.
namespace autowall {

struct Shot {
    float damage    = 0.f;   // after falloff, armour and the wall model
    float freeAir   = 0.f;   // same shot with no walls assumed
    int   wallsUsed = 0;
    bool  lethal    = false;
};

// `head` selects the headshot multiplier and the helmet reduction.
Shot Estimate(const weapon::Info& info, float distance, bool head,
              const weapon::Armor& armor, int targetHealth,
              bool assumeWalls, int walls);

// Hitchance moved out of this file: spread is not a damage question and it
// now lives in features/rage/hitchance.h as its own component.

} // namespace autowall
