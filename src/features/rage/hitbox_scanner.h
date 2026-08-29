#pragma once
#include "core/globals.h"
#include "sdk/math.h"

// Turns the hitbox checkboxes into the concrete list of bones the point
// scanner will sample.
//
// Headshot-only and force-body are applied HERE, while the list is being
// built, not by scanning everything and discarding results afterwards. That is
// the difference between "the scanner never considered the head" and "the
// scanner wasted work on points it was always going to throw away".
namespace rage {
namespace hitboxes {

struct Entry {
    int   hitbox = -1;      // cfg::RageHitbox
    int   bone   = -1;      // bones::Id
    float radius = 5.f;     // nominal hitbox radius in Source units
    bool  head   = false;
    bool  limb   = false;
};

// Room for every hitbox, counting the paired ones (arms, legs, feet) twice.
constexpr int kMaxEntries = 16;

// Fills `out` with the allowed bones and returns how many were written.
// `forceBodyHeld` is the live state of the force-body key, which overrides the
// config's own forceBody flag for as long as it is down.
int Build(const cfg::RageWeaponConfig& weaponCfg, bool forceBodyHeld,
          Entry* out, int max);

// "Head", "Stomach", ... Indexed by cfg::RageHitbox.
const char* Name(int hitbox);
extern const char* const kNames[cfg::HitboxCount];

} // namespace hitboxes
} // namespace rage
