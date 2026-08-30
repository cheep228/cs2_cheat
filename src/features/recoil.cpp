#include <Windows.h>

#include <cmath>

#include "core/globals.h"
#include "features/recoil.h"
#include "sdk/entity.h"
#include "sdk/interfaces.h"
#include "sdk/offsets.h"
#include "sdk/weapon.h"

namespace recoil {

Stats stats;

namespace {

// Punch as it stood on the previous command, so the correction can follow the
// CHANGE rather than the absolute value. Reset whenever the player stops
// shooting, which is what keeps the view from jerking when the trigger is
// released - the classic bug where letting go snaps the camera.
Vector3 g_prevPunch{};
bool    g_havePrev = false;
int     g_prevShots = 0;

// Correction that smoothing has not handed over yet.
float g_pendingPitch = 0.f;
float g_pendingYaw   = 0.f;

void Reset(const char* why) {
    g_prevPunch = Vector3{};
    g_havePrev = false;
    g_prevShots = 0;
    g_pendingPitch = 0.f;
    g_pendingYaw = 0.f;
    stats.running = false;
    stats.appliedPitch = 0.f;
    stats.appliedYaw = 0.f;
    stats.pendingPitch = 0.f;
    stats.pendingYaw = 0.f;
    stats.reason = why;
}

// Only weapons that actually walk. A knife, a grenade or the C4 has no recoil
// to compensate, and the ballistics table is what already knows the difference.
bool HasRecoil(const weapon::Active& gun) {
    if (!gun.ok || !gun.info)
        return false;
    if (gun.info->damage <= 0.f)
        return false;

    switch (gun.info->kind) {
    case weapon::Kind::Pistol:
    case weapon::Kind::Smg:
    case weapon::Kind::Rifle:
    case weapon::Kind::Sniper:
    case weapon::Kind::Machinegun:
    case weapon::Kind::Shotgun:
        return true;
    default:
        return false;   // knife and anything without ballistics
    }
}

} // namespace

void Run(usercmd::UserCmd* cmd, uintptr_t localPawn) {
    const cfg::RecoilControl& rc = cfg::c.recoil;
    stats.enabled = rc.enabled;

    if (!rc.enabled || !cmd || !g::client) {
        Reset("disabled");
        return;
    }
    if (!mem::Valid(localPawn) ||
        mem::Read<uint8_t>(localPawn + schema::m_lifeState) != 0) {
        Reset("local dead");
        return;
    }

    if (const ifaces::GlobalVars* gv = ifaces::Globals())
        stats.tickCount = gv->tickCount;

    const weapon::Active gun = weapon::Local(localPawn);
    stats.weapon = (gun.ok && gun.info) ? gun.info->name : "none";
    if (!HasRecoil(gun)) {
        Reset("weapon has no recoil");
        return;
    }

    const int shots = mem::Read<int32_t>(localPawn + schema::m_iShotsFired);
    stats.shotsFired = shots;

    // The real recoil state, straight from the pawn. If this stays at zero
    // while spraying, the field is wrong and nothing below is trustworthy -
    // which is exactly what the panel reports rather than papering over.
    const Vector3 punch = mem::Read<Vector3>(localPawn + schema::m_aimPunchAngle);
    stats.currentPunch = punch;

    // Stopped shooting: forget the history so the next burst starts clean and
    // no leftover correction is applied to a view nobody is spraying with.
    if (shots <= 0) {
        Reset("not firing");
        stats.currentPunch = punch;
        return;
    }

    // Leave the opening shots alone - they are the accurate ones, and the
    // spray only becomes systematic after them.
    if (shots < rc.startAfterShots) {
        g_prevPunch = punch;
        g_havePrev = true;
        g_prevShots = shots;
        stats.running = false;
        stats.previousPunch = punch;
        stats.deltaPunch = Vector3{};
        stats.appliedPitch = 0.f;
        stats.appliedYaw = 0.f;
        stats.reason = "waiting for start shot";
        return;
    }

    // First corrected command of a burst: seed the history, correct nothing.
    // Correcting against a stale punch here is what produces a visible jump.
    if (!g_havePrev || shots < g_prevShots) {
        g_prevPunch = punch;
        g_havePrev = true;
        g_prevShots = shots;
        stats.running = false;
        stats.previousPunch = punch;
        stats.deltaPunch = Vector3{};
        stats.appliedPitch = 0.f;
        stats.appliedYaw = 0.f;
        stats.reason = "seeding burst";
        return;
    }

    stats.previousPunch = g_prevPunch;

    // How much the punch moved since the last command. Following the change
    // rather than the absolute punch is what keeps the correction additive
    // across a spray instead of fighting itself every tick.
    const Vector3 delta{ punch.x - g_prevPunch.x,
                         punch.y - g_prevPunch.y,
                         punch.z - g_prevPunch.z };
    stats.deltaPunch = delta;

    g_prevPunch = punch;
    g_prevShots = shots;

    const float vert = static_cast<float>(rc.vertical) / 100.f;
    const float horz = static_cast<float>(rc.horizontal) / 100.f;

    // The punch is a QAngle: x is pitch, y is yaw. Pulling the view back means
    // subtracting the change, scaled per axis.
    g_pendingPitch += -delta.x * vert * rc.scale;
    g_pendingYaw   += -delta.y * horz * rc.scale;

    // Smoothing hands over a fraction of what is owed each COMMAND. Driven by
    // the command pipeline, so it behaves the same at any frame rate, and
    // nothing sleeps.
    float applyPitch = g_pendingPitch;
    float applyYaw   = g_pendingYaw;
    if (rc.smooth > 0) {
        const float f = 1.f / (1.f + static_cast<float>(rc.smooth));
        applyPitch = g_pendingPitch * f;
        applyYaw   = g_pendingYaw * f;
    }

    g_pendingPitch -= applyPitch;
    g_pendingYaw   -= applyYaw;

    if (std::fabs(applyPitch) < 1e-5f && std::fabs(applyYaw) < 1e-5f) {
        stats.running = false;
        stats.appliedPitch = 0.f;
        stats.appliedYaw = 0.f;
        stats.pendingPitch = g_pendingPitch;
        stats.pendingYaw = g_pendingYaw;
        stats.reason = "nothing to correct";
        return;
    }

    // ---- the single final angle -------------------------------------------
    // Read what is there NOW, which already includes anything the aimbot wrote
    // this frame, add the correction, write once. Two independent writers
    // fighting over the view is exactly what this ordering avoids.
    const Vector2 raw = mem::Read<Vector2>(g::client + offsets::dwViewAngles);

    Angle out;
    out.pitch = ClampPitch(raw.x + applyPitch);
    out.yaw   = NormalizeYaw(raw.y + applyYaw);

    const Vector2 written{ out.pitch, out.yaw };
    mem::Write<Vector2>(g::client + offsets::dwViewAngles, written);

    stats.running = true;
    stats.appliedPitch = applyPitch;
    stats.appliedYaw = applyYaw;
    stats.pendingPitch = g_pendingPitch;
    stats.pendingYaw = g_pendingYaw;
    stats.reason = "correcting";
}

} // namespace recoil
