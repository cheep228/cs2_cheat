#pragma once
#include "core/globals.h"
#include "features/autowall.h"
#include "sdk/weapon.h"

// The minimum-damage rule, in one place.
//
// Keeping this out of the scanner matters because the rule is not "compare
// against a slider": the override key swaps the value while it is held, and a
// value above 100 is read relative to the target's health. Spreading those two
// branches through the point loop is how a ragebot ends up rejecting shots for
// reasons nobody can reconstruct afterwards.
namespace rage {
namespace damage {

// Damage a point must reach to be allowed.
//
//   value <= 100 : that many points of damage, flat.
//   value  > 100 : the target's current HP plus (value - 100), which is how a
//                  "kill with 10 points to spare" setting is expressed.
//
// `overrideHeld` swaps minDamage for minDamageOverride for as long as the
// override key is down.
int Required(const cfg::RageWeaponConfig& weaponCfg, bool overrideHeld, int enemyHealth);

// The raw damage estimate for one point. Thin wrapper over autowall::Estimate
// so the scanner never has to know about the wall model's parameters.
autowall::Shot Estimate(const weapon::Info& info, float distance, bool head,
                        const weapon::Armor& armor, int enemyHealth);

} // namespace damage
} // namespace rage
