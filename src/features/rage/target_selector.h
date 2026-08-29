#pragma once
#include <cstdint>

#include "core/globals.h"
#include "features/rage/types.h"
#include "sdk/math.h"

// Choosing who to shoot and where.
//
// Two separate decisions live here, in this order:
//   1. Collect() gathers every enemy worth scanning. It filters and nothing
//      else - it never picks a winner.
//   2. Pick() runs after every collected enemy has been fully scanned, so
//      "highest damage" can actually compare damage instead of guessing.
//
// Committing to the first acceptable player, which is the usual shortcut, makes
// every target mode except "closest FOV" meaningless.
namespace rage {
namespace targets {

constexpr int kMaxTargets = 64;

// Live enemies that are alive, non-dormant, on another team and inside the FOV
// limit. Returns the count written to `out`; `outCandidates` receives the
// number that passed everything except the FOV test, for the debug panel.
int Collect(uintptr_t entityList, uintptr_t localPawn, int localTeam,
            const Vector3& eye, const Angle& view, float maxFov,
            Target* out, int max, int* outCandidates);

// Ranks one point on one target. Higher is better. Lethal shots, damage,
// hitbox preference and how central the sample is all feed in; the weights are
// documented at the implementation.
float ScorePoint(const AimPoint& point, const cfg::RageWeaponConfig& weaponCfg,
                 int enemyHealth);

// Best solution for the configured target mode, or null when none are valid.
const Solution* Pick(const Solution* solutions, int count, int mode);

} // namespace targets
} // namespace rage
