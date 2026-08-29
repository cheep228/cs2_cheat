#pragma once
#include "core/globals.h"
#include "features/rage/hitbox_scanner.h"
#include "sdk/math.h"

// Multipoint sampling: turns one bone position into the handful of world
// points worth testing on it.
//
// The samples are laid out in a frame built from the shooter's line of sight,
// so "left" and "right" are left and right as seen down the barrel rather than
// along some fixed world axis. Sampling along world X/Y instead makes the
// spread collapse to a line whenever the enemy happens to stand on that axis.
namespace rage {
namespace points {

// Head gets four samples, torso three, limbs one. Deliberately small: a dozen
// points per hitbox costs bone reads and damage estimates for pairs of samples
// that sit fractions of a unit apart and never change the outcome.
constexpr int kMaxPerHitbox = 4;

// Writes the samples for one hitbox and returns how many. out[0] is always the
// centre, so a clean centre shot is never passed over for an edge point.
int Generate(const hitboxes::Entry& box, const Vector3& centre, const Vector3& eye,
             const cfg::RageWeaponConfig& weaponCfg, Vector3* out, int max);

} // namespace points
} // namespace rage
