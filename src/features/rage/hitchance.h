#pragma once
#include "sdk/weapon.h"

// How likely the shot is to land, as its own component.
//
// This is NOT rand() % 100. It reads the weapon's live accuracy penalty out of
// the game, adds the per-class cone at rest, widens it for movement, air time
// and reloading, and compares the resulting spread disc against the size of the
// hitbox being aimed at. A higher setting therefore means "wait for a genuinely
// tighter weapon state", not "roll a bigger number".
//
// HONEST LIMIT: the exact per-weapon inaccuracy curves live in the game's
// weapon scripts, which a read-only overlay does not parse. The class cone in
// sdk/weapon.cpp is an approximation, so treat the output as a well-founded
// estimate rather than the engine's own number.
namespace rage {
namespace hitchance {

struct Input {
    float distance   = 0.f;   // eye to aim point, Source units
    float hitRadius  = 0.f;   // nominal radius of the hitbox being aimed at
    float localSpeed = 0.f;   // horizontal speed of the shooter
    bool  onGround   = true;
};

// 0..100. Returns 0 when there is no weapon data to reason about.
float Estimate(const weapon::Active& gun, const Input& in);

// The current spread cone in tangent units, exposed so the debug panel can
// show why a chance came out low.
float Cone(const weapon::Active& gun, float localSpeed, bool onGround);

} // namespace hitchance
} // namespace rage
