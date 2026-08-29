#include <cmath>

#include "features/rage/hitchance.h"

namespace rage {
namespace hitchance {
namespace {

// Cone at rest for the class, in tangent units.
float BaseCone(const weapon::Active& gun) {
    if (!gun.info)
        return 0.02f;

    float cone = gun.info->baseSpread;

    // An unscoped sniper is not a weapon that has a hitchance, it is a coin
    // flip; saying so keeps the gate from ever passing one.
    if (gun.info->kind == weapon::Kind::Sniper && !gun.scoped)
        cone = 0.04f;

    return cone;
}

} // namespace

float Cone(const weapon::Active& gun, float localSpeed, bool onGround) {
    float cone = BaseCone(gun) + gun.accuracyPenalty;

    // Movement and air time dominate every other term, which is exactly why
    // auto-stop exists: it is the only one of these the ragebot can change.
    if (!onGround)
        cone *= 4.5f;
    else if (localSpeed > 130.f)
        cone *= 2.6f;
    else if (localSpeed > 60.f)
        cone *= 1.5f;

    if (gun.reloading)
        cone *= 2.f;

    return cone;
}

float Estimate(const weapon::Active& gun, const Input& in) {
    if (!gun.ok || !gun.info || in.distance <= 1.f || in.hitRadius <= 0.f)
        return 0.f;

    const float cone = Cone(gun, in.localSpeed, in.onGround);
    if (cone <= 1e-6f)
        return 100.f;

    // Shots land inside a disc of radius cone*distance; the hitbox covers a
    // disc of hitRadius. The area ratio is the share that connects.
    const float spreadRadius = cone * in.distance;
    if (spreadRadius <= in.hitRadius)
        return 100.f;

    const float ratio = in.hitRadius / spreadRadius;
    const float chance = ratio * ratio * 100.f;
    return chance > 100.f ? 100.f : chance;
}

} // namespace hitchance
} // namespace rage
