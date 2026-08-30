#include <Windows.h>

#include <cmath>
#include <cstdio>

#include "core/globals.h"
#include "core/log.h"
#include "features/rage/damage.h"
#include "features/rage/fire.h"
#include "features/rage/hitbox_scanner.h"
#include "features/rage/hitchance.h"
#include "features/rage/point_scanner.h"
#include "features/rage/target_selector.h"
#include "features/rage/types.h"
#include "features/rage/weapon_settings.h"
#include "features/ragebot.h"
#include "sdk/bones.h"
#include "sdk/entity.h"
#include "sdk/offsets.h"
#include "sdk/trace.h"
#include "sdk/weapon.h"

namespace rage {

const char* StageName(Stage s) {
    switch (s) {
    case Stage::Idle:            return "idle";
    case Stage::Disabled:        return "disabled";
    case Stage::KeyUp:           return "key not held";
    case Stage::NoLocalPawn:     return "no local pawn";
    case Stage::NoBones:         return "bones not calibrated";
    case Stage::NoWeapon:        return "no gun with ballistics";
    case Stage::WeaponGroupOff:  return "weapon group disabled";
    case Stage::NoHitbox:        return "no hitbox selected";
    case Stage::NoCandidates:    return "no live enemies";
    case Stage::NoneInFov:       return "no enemy inside FOV";
    case Stage::NoBonePositions: return "bones unreadable on target";
    case Stage::BelowDamage:     return "below minimum damage";
    case Stage::BelowHitchance:  return "below hitchance";
    case Stage::AutoStopWaiting: return "auto stop: slowing down";
    case Stage::Firing:          return "aiming";
    }
    return "?";
}

} // namespace rage

namespace {

using rage::AimPoint;
using rage::Solution;
using rage::Stage;
using rage::Target;

constexpr int kMaxSolutions = 32;

float LocalSpeed(uintptr_t pawn) {
    const Vector3 v = mem::Read<Vector3>(pawn + schema::m_vecVelocity);
    return std::sqrt(v.x * v.x + v.y * v.y);
}

bool OnGround(uintptr_t pawn) {
    return (mem::Read<uint32_t>(pawn + schema::m_fFlags) & 1u) != 0;
}

bool KeyHeld(int vk) {
    if (vk == 0)
        return true;   // 0 means always on
    return (GetAsyncKeyState(vk) & 0x8000) != 0;
}

// Everything one call to ScanTarget needs, bundled so the signature stays
// readable and no parameter can be passed in the wrong order.
struct ScanContext {
    const cfg::RageWeaponConfig* weaponCfg = nullptr;
    const weapon::Active*        gun       = nullptr;
    const rage::hitboxes::Entry* boxes     = nullptr;
    int                          boxCount  = 0;

    Vector3 eye{};
    Angle   view{};
    float   speed    = 0.f;
    bool    onGround = true;
    bool    overrideHeld = false;
    uintptr_t localPawn = 0;

    Matrix4x4 viewMatrix{};
};

// Records a sample for the debug overlay. Rejected points are kept too - the
// whole point of drawing them is seeing what the scanner threw away.
void RecordDrawn(ragebot::State& out, const Matrix4x4& vm, const Vector3& world,
                 bool passed, int* index) {
    *index = -1;
    if (!cfg::c.rage.drawPoints || out.drawnCount >= ragebot::kMaxDrawnPoints)
        return;

    ragebot::DrawnPoint& dp = out.drawn[out.drawnCount];
    if (!WorldToScreen(vm, world, dp.screen, g::screenW, g::screenH))
        return;

    dp.passed = passed;
    dp.chosen = false;
    *index = out.drawnCount++;
}

// Scans one enemy completely and returns its best point. Nothing here compares
// this target against any other - that is Pick's job, after every target has
// been through this function.
Solution ScanTarget(const Target& target, const ScanContext& ctx,
                    ragebot::State& out, int* bestDrawnIndex) {
    Solution solution;
    solution.target = target;

    const cfg::RageWeaponConfig& wc = *ctx.weaponCfg;
    const int required = rage::damage::Required(wc, ctx.overrideHeld, target.health);

    float bestScore = 0.f;
    bool  haveBest = false;

    for (int b = 0; b < ctx.boxCount; ++b) {
        const rage::hitboxes::Entry& box = ctx.boxes[b];

        const Vector3 centre = bones::AimPoint(target.pawn, box.bone);
        if (centre.IsZero())
            continue;

        Vector3 samples[rage::points::kMaxPerHitbox];
        const int sampleCount =
            rage::points::Generate(box, centre, ctx.eye, wc, samples,
                                   rage::points::kMaxPerHitbox);

        for (int s = 0; s < sampleCount; ++s) {
            const Vector3& world = samples[s];
            ++solution.pointsTested;

            const Angle angle = CalcAngle(ctx.eye, world);
            const float fov = AngleDelta(angle, ctx.view);
            if (fov > wc.fov)
                continue;   // outside the cone; not a rejection worth reporting

            const float distance = (world - ctx.eye).Length();

            const autowall::Shot shot =
                rage::damage::Estimate(*ctx.gun->info, distance, box.head,
                                       target.armor, target.health);

            rage::hitchance::Input hcIn;
            hcIn.distance = distance;
            hcIn.hitRadius = box.radius;
            hcIn.localSpeed = ctx.speed;
            hcIn.onGround = ctx.onGround;
            const float chance = rage::hitchance::Estimate(*ctx.gun, hcIn);

            if (shot.damage > solution.bestDamageSeen)
                solution.bestDamageSeen = shot.damage;
            if (chance > solution.bestHitchanceSeen)
                solution.bestHitchanceSeen = chance;

            const bool damageOk = required <= 0 ||
                                  shot.damage >= static_cast<float>(required);
            const bool chanceOk = chance >= static_cast<float>(wc.hitchance);
            const bool passed = damageOk && chanceOk;

            int drawnIndex = -1;
            RecordDrawn(out, ctx.viewMatrix, world, passed, &drawnIndex);

            if (!passed)
                continue;

            ++solution.pointsPassed;

            AimPoint point;
            point.position  = world;
            point.angle     = angle;
            point.hitbox    = box.hitbox;
            point.bone      = box.bone;
            point.isHead    = box.head;
            point.isLimb    = box.limb;
            point.isCentre  = (s == 0);
            point.distance  = distance;
            point.fov       = fov;
            point.damage    = shot.damage;
            point.freeAir   = shot.freeAir;
            point.hitchance = chance;
            point.lethal    = shot.lethal;

            // Visibility is unknown until a trace backend exists; the scorer is
            // told that rather than being handed an invented boolean.
            const trace::Result los =
                trace::LineOfSight(ctx.eye, world, ctx.localPawn);
            point.visibilityKnown = trace::Available() && los != trace::Result::Unknown;
            point.visible = !point.visibilityKnown || los == trace::Result::Visible;
            if (point.visibilityKnown && !point.visible)
                continue;

            point.score = rage::targets::ScorePoint(point, wc, target.health);

            if (!haveBest || point.score > bestScore) {
                haveBest = true;
                bestScore = point.score;
                solution.point = point;
                solution.valid = true;
                *bestDrawnIndex = drawnIndex;
            }
        }
    }

    return solution;
}

} // namespace

namespace ragebot {

State state;

void Run() {
    State fresh;
    rage::Diagnostics& d = fresh.diag;

    const cfg::Ragebot& r = cfg::c.rage;

    // --- 1. enabled ------------------------------------------------------
    if (!cfg::c.masterSwitch || !r.enabled || !g::client) {
        d.stage = Stage::Disabled;
        d.result = "IDLE";
        rage::fire::Reset();
        state = fresh;
        return;
    }

    if (!KeyHeld(r.key)) {
        d.stage = Stage::KeyUp;
        d.result = "IDLE";
        rage::fire::Reset();
        state = fresh;
        return;
    }

    d.overrideHeld = r.minDamageOverrideKey != 0 &&
                     (GetAsyncKeyState(r.minDamageOverrideKey) & 0x8000) != 0;
    d.forceBodyHeld = r.forceBodyKey != 0 &&
                      (GetAsyncKeyState(r.forceBodyKey) & 0x8000) != 0;

    // --- 2. local player -------------------------------------------------
    const uintptr_t entityList = mem::Read<uintptr_t>(g::client + offsets::dwEntityList);
    const uintptr_t localPawn = mem::Read<uintptr_t>(g::client + offsets::dwLocalPlayerPawn);
    if (!mem::Valid(entityList) || !mem::Valid(localPawn)) {
        d.stage = Stage::NoLocalPawn;
        d.result = "REJECTED - NO LOCAL PLAYER";
        state = fresh;
        return;
    }

    if (!bones::layout.calibrated) {
        d.stage = Stage::NoBones;
        d.result = "REJECTED - NO BONES";
        state = fresh;
        return;
    }

    fresh.active = true;

    // --- 3. weapon -------------------------------------------------------
    const weapon::Active gun = weapon::Local(localPawn);
    if (!gun.ok || !gun.info || gun.info->damage <= 0.f) {
        d.stage = Stage::NoWeapon;
        d.result = "REJECTED - NO WEAPON";
        state = fresh;
        return;
    }
    d.weapon = gun.info->name;

    int group = cfg::GroupGlobal;
    const cfg::RageWeaponConfig& wc = rage::weapons::Active(gun.defIndex, &group);
    d.group = rage::weapons::GroupName(group);
    d.groupActive = (group != cfg::GroupGlobal);
    d.maxFov = wc.fov;
    d.requiredHitchance = wc.hitchance;
    fresh.maxFov = wc.fov;

    // --- 4. allowed hitboxes --------------------------------------------
    rage::hitboxes::Entry boxes[rage::hitboxes::kMaxEntries];
    const int boxCount = rage::hitboxes::Build(wc, d.forceBodyHeld, boxes,
                                               rage::hitboxes::kMaxEntries);
    if (boxCount == 0) {
        d.stage = Stage::NoHitbox;
        d.result = "REJECTED - NO HITBOX";
        state = fresh;
        return;
    }

    // --- 5. candidates ---------------------------------------------------
    const int localTeam = mem::Read<uint8_t>(localPawn + schema::m_iTeamNum);
    const Vector3 eye = sdk::GetAbsOrigin(localPawn) +
                        mem::Read<Vector3>(localPawn + schema::m_vecViewOffset);

    Angle view;
    {
        const Vector2 raw = mem::Read<Vector2>(g::client + offsets::dwViewAngles);
        view.pitch = raw.x;
        view.yaw = raw.y;
    }

    Target targets[rage::targets::kMaxTargets];
    const int targetCount =
        rage::targets::Collect(entityList, localPawn, localTeam, eye, view, wc.fov,
                               targets, rage::targets::kMaxTargets, &d.candidates);
    d.inFov = targetCount;

    if (d.candidates == 0) {
        d.stage = Stage::NoCandidates;
        d.result = "REJECTED - NO ENEMIES";
        state = fresh;
        return;
    }
    if (targetCount == 0) {
        d.stage = Stage::NoneInFov;
        d.result = "REJECTED - FOV";
        state = fresh;
        return;
    }

    // --- 6..9. scan every target, then choose ----------------------------
    ScanContext ctx;
    ctx.weaponCfg = &wc;
    ctx.gun = &gun;
    ctx.boxes = boxes;
    ctx.boxCount = boxCount;
    ctx.eye = eye;
    ctx.view = view;
    ctx.speed = LocalSpeed(localPawn);
    ctx.onGround = OnGround(localPawn);
    ctx.overrideHeld = d.overrideHeld;
    ctx.localPawn = localPawn;
    ctx.viewMatrix = mem::Read<Matrix4x4>(g::client + offsets::dwViewMatrix);

    Solution solutions[kMaxSolutions];
    int drawnForSolution[kMaxSolutions];
    int solutionCount = 0;

    float bestDamageSeen = 0.f;
    float bestChanceSeen = 0.f;
    bool  anyBoneRead = false;

    const int scanCount = targetCount < kMaxSolutions ? targetCount : kMaxSolutions;
    for (int i = 0; i < scanCount; ++i) {
        drawnForSolution[solutionCount] = -1;
        Solution s = ScanTarget(targets[i], ctx, fresh,
                                &drawnForSolution[solutionCount]);

        d.pointsTested += s.pointsTested;
        d.pointsPassed += s.pointsPassed;
        if (s.pointsTested > 0)
            anyBoneRead = true;
        if (s.bestDamageSeen > bestDamageSeen)
            bestDamageSeen = s.bestDamageSeen;
        if (s.bestHitchanceSeen > bestChanceSeen)
            bestChanceSeen = s.bestHitchanceSeen;

        solutions[solutionCount++] = s;
    }

    const Solution* best = rage::targets::Pick(solutions, solutionCount, r.targetMode);

    if (!best) {
        // Report the gate that actually stopped the shot, not just "no target".
        if (!anyBoneRead) {
            d.stage = Stage::NoBonePositions;
            d.result = "REJECTED - NO BONES ON TARGET";
        } else if (bestDamageSeen > 0.f &&
                   bestDamageSeen < static_cast<float>(
                       rage::damage::Required(wc, d.overrideHeld, targets[0].health))) {
            d.stage = Stage::BelowDamage;
            d.damage = bestDamageSeen;
            d.requiredDamage = rage::damage::Required(wc, d.overrideHeld, targets[0].health);
            d.result = "REJECTED - DAMAGE";
        } else if (bestChanceSeen < static_cast<float>(wc.hitchance)) {
            d.stage = Stage::BelowHitchance;
            d.hitchance = bestChanceSeen;
            d.result = "REJECTED - HITCHANCE";
        } else {
            d.stage = Stage::NoneInFov;
            d.result = "REJECTED - FOV";
        }
        state = fresh;
        return;
    }

    // --- winning solution ------------------------------------------------
    const AimPoint& point = best->point;

    fresh.hasTarget = true;
    d.hasTarget = true;
    d.targetSlot = best->target.slot;
    d.targetHealth = best->target.health;
    snprintf(d.targetName, sizeof(d.targetName), "%s", best->target.name);

    d.hitbox = point.hitbox;
    d.damage = point.damage;
    d.requiredDamage = rage::damage::Required(wc, d.overrideHeld, best->target.health);
    d.hitchance = point.hitchance;
    d.fov = point.fov;
    d.lethal = point.lethal;
    d.visibilityKnown = point.visibilityKnown;

    WorldToScreen(ctx.viewMatrix, point.position, fresh.targetScreen,
                  g::screenW, g::screenH);

    const int chosenDrawn = drawnForSolution[static_cast<int>(best - solutions)];
    if (chosenDrawn >= 0 && chosenDrawn < fresh.drawnCount)
        fresh.drawn[chosenDrawn].chosen = true;

    // --- 12. aim ---------------------------------------------------------
    rage::fire::WriteViewAngles(point.angle);

    // --- 13. fire --------------------------------------------------------
    // Auto stop only runs now, with a point already chosen. Running it every
    // frame would just be a movement blocker wearing a ragebot's name.
    const bool movementReady = rage::fire::AutoStop(wc.autoStop, gun, ctx.speed);
    if (!movementReady) {
        d.stage = Stage::AutoStopWaiting;
        d.result = "WAITING - AUTO STOP";
        state = fresh;
        return;
    }

    if (wc.autoScope && rage::fire::NeedsScope(gun)) {
        rage::fire::ToggleScope();
        d.stage = Stage::Firing;
        d.result = "WAITING - SCOPING";
        state = fresh;
        return;
    }

    d.stage = Stage::Firing;
    d.result = "VALID";

    // With Auto Fire off the ragebot has still done everything except pull the
    // trigger: the view is on the point and the panel says the shot is valid.
    if (wc.autoFire)
        rage::fire::Shoot(gun);

    state = fresh;
}

void Draw(ImDrawList* d, int screenW, int screenH) {
    const cfg::Ragebot& r = cfg::c.rage;
    if (!cfg::c.masterSwitch || !r.enabled || !r.debug)
        return;

    const ImVec2 centre(screenW * 0.5f, screenH * 0.5f);

    if (r.drawFov && state.maxFov > 0.f && state.maxFov < 180.f) {
        const float pixelsPerDegree = screenH / 60.f;
        d->AddCircle(centre, state.maxFov * pixelsPerDegree,
                     ImGui::ColorConvertFloat4ToU32(r.fovColor), 64, 1.f);
    }

    if (r.drawPoints) {
        const ImU32 pass = ImGui::ColorConvertFloat4ToU32(r.pointColor);
        const ImU32 fail = IM_COL32(220, 70, 70, 150);
        for (int i = 0; i < state.drawnCount; ++i) {
            const DrawnPoint& p = state.drawn[i];
            const ImVec2 at(p.screen.x, p.screen.y);
            if (p.chosen)
                d->AddCircle(at, 4.5f, pass, 12, 1.6f);
            d->AddCircleFilled(at, p.passed ? 2.0f : 1.4f, p.passed ? pass : fail);
        }
    }

    if (r.drawTarget && state.hasTarget) {
        const ImU32 col = ImGui::ColorConvertFloat4ToU32(r.targetColor);
        const ImVec2 t(state.targetScreen.x, state.targetScreen.y);
        d->AddCircleFilled(t, 3.5f, col);
        d->AddLine(centre, t, col, 1.1f);

        // The numbers that explain the decision, next to the point they explain.
        char label[96];
        snprintf(label, sizeof(label), "%s  %.0f dmg%s",
                 rage::hitboxes::Name(state.diag.hitbox), state.diag.damage,
                 state.diag.lethal ? "  LETHAL" : "");
        d->AddText(ImVec2(t.x + 7.f, t.y - 6.f), col, label);
    }
}

} // namespace ragebot
