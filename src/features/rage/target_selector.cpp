#include <cmath>
#include <cstdio>
#include <cstring>

#include "features/rage/target_selector.h"
#include "sdk/entity.h"
#include "sdk/offsets.h"

namespace rage {
namespace targets {
namespace {

// Half-height of a standing player, used to aim the coarse FOV test at the
// chest rather than at the feet.
constexpr float kChestOffset = 48.f;

// Radius of the sphere that comfortably contains a player model. The coarse
// FOV filter subtracts the angle this subtends, so an enemy whose head is
// inside the FOV cone is never dropped just because their centre is outside it.
constexpr float kBoundingRadius = 42.f;

bool IsDormant(uintptr_t pawn) {
    const uintptr_t node = mem::Read<uintptr_t>(pawn + schema::m_pGameSceneNode);
    if (!mem::Valid(node))
        return true;   // no scene node means nothing to aim at
    return mem::Read<bool>(node + schema::m_bDormant);
}

float AngularRadius(float distance) {
    if (distance <= 1.f)
        return 180.f;
    return std::atan(kBoundingRadius / distance) * kRad2Deg;
}

} // namespace

int Collect(uintptr_t entityList, uintptr_t localPawn, int localTeam,
            const Vector3& eye, const Angle& view, float maxFov,
            Target* out, int max, int* outCandidates) {
    int written = 0;
    int candidates = 0;

    for (int slot = 1; slot <= kMaxTargets && written < max; ++slot) {
        sdk::PlayerInfo info;

        // GetPlayer already rejects dead players, broken handles and bogus
        // health, so this loop only has to add what it does not cover.
        if (!sdk::GetPlayer(entityList, slot, info))
            continue;
        if (info.pawn == localPawn)
            continue;
        if (info.team == localTeam)
            continue;
        if (IsDormant(info.pawn))
            continue;

        ++candidates;

        const Vector3 centre{ info.origin.x, info.origin.y, info.origin.z + kChestOffset };
        const float distance = (centre - eye).Length();
        const float fov = AngleDelta(CalcAngle(eye, centre), view);

        // Coarse gate. The exact per-point FOV is checked again during the
        // scan; this only avoids scanning enemies that cannot possibly qualify.
        if (fov - AngularRadius(distance) > maxFov)
            continue;

        Target& t = out[written++];
        t.slot     = slot;
        t.pawn     = info.pawn;
        t.health   = info.health;
        t.team     = info.team;
        t.origin   = info.origin;
        t.distance = distance;
        t.fov      = fov;
        t.armor    = weapon::ArmorOf(info.pawn);

        snprintf(t.name, sizeof(t.name), "%s", info.name.c_str());
    }

    if (outCandidates)
        *outCandidates = candidates;
    return written;
}

float ScorePoint(const AimPoint& point, const cfg::RageWeaponConfig& weaponCfg,
                 int enemyHealth) {
    // The weights below are a ranking, not physics. What matters is the order
    // they impose: a shot that kills beats a shot that hurts, damage beats
    // hitbox taste, and hitbox taste beats how central the sample is.
    float score = 0.f;

    // 1. Killing outright dominates everything else.
    if (point.lethal)
        score += 1000.f;

    // 2. Damage, normalised so a full-health kill is worth about 100.
    const int health = enemyHealth > 0 ? enemyHealth : 100;
    score += (point.damage / static_cast<float>(health)) * 100.f;

    // 3. Hitbox preference. Prefer-body raises the body without banning the
    //    head; force-body never reaches here because the head was excluded
    //    while the hitbox list was being built.
    if (weaponCfg.preferBody) {
        score += point.isHead ? -25.f : 20.f;
    } else {
        score += point.isHead ? 30.f : 0.f;
    }
    if (point.isLimb)
        score -= 35.f;   // limbs are last resort even when enabled

    // 4. Body-aim-if-lethal: a body shot that already kills is worth more than
    //    a head shot that also kills, because it is the larger target.
    if (weaponCfg.bodyAimLethal && point.lethal && !point.isHead)
        score += 60.f;

    // 5. Centre samples are safer: the model can move between the bone read and
    //    the shot, and an edge point is the first thing to fall off it.
    if (point.isCentre)
        score += 12.f;

    // 6. Confidence in the shot itself.
    score += point.hitchance * 0.15f;

    // 7. All else equal, take the point that needs the smaller flick.
    score -= point.fov * 0.5f;

    // 8. A point confirmed visible would earn a bonus here. No trace backend
    //    exists (sdk/trace.h), so nothing is awarded rather than pretending.
    if (point.visibilityKnown && point.visible)
        score += 40.f;

    return score;
}

const Solution* Pick(const Solution* solutions, int count, int mode) {
    const Solution* best = nullptr;
    float bestKey = 0.f;

    for (int i = 0; i < count; ++i) {
        const Solution& s = solutions[i];
        if (!s.valid)
            continue;

        // Lower key wins for every mode; the two "highest" modes negate.
        float key;
        switch (mode) {
        case cfg::TargetLowestHp:
            key = static_cast<float>(s.target.health) * 1000.f + s.point.fov;
            break;
        case cfg::TargetClosestDistance:
            key = s.target.distance;
            break;
        case cfg::TargetHighestDamage:
            key = -s.point.damage * 1000.f + s.point.fov;
            break;
        case cfg::TargetClosestFov:
        default:
            key = s.point.fov;
            break;
        }

        if (!best || key < bestKey) {
            best = &s;
            bestKey = key;
        }
    }
    return best;
}

} // namespace targets
} // namespace rage
