#include <Windows.h>

#include <cmath>
#include <cstdio>

#include "core/globals.h"
#include "core/log.h"
#include "features/triggerbot.h"
#include "sdk/bones.h"
#include "sdk/entity.h"
#include "sdk/entitylist.h"
#include "sdk/interfaces.h"
#include "sdk/offsets.h"
#include "sdk/visibility.h"
#include "sdk/weapon.h"

namespace triggerbot {

Stats stats;

namespace {

constexpr int kMaxPlayers = 64;

// How far the ray reaches. Past this the shot would not connect anyway.
constexpr float kRayLength = 8192.f;

// One capsule of the player's hit volume: a segment with a radius. A single
// bone becomes a degenerate capsule - a sphere - by using the same point twice.
struct Capsule {
    Vector3 a{};
    Vector3 b{};
    float   radius = 0.f;
    int     group  = -1;
};

// The hit volume, built from the live skeleton. These are the same bones the
// ESP draws, so what the trigger tests and what you see are the same thing.
int BuildCapsules(uintptr_t pawn, Capsule* out, int max) {
    auto bone = [&](int id) { return bones::Position(pawn, id); };

    struct Def { int a, b; float r; int group; };
    const Def defs[] = {
        { bones::Head,      bones::Head,      3.9f, GroupHead },
        { bones::Neck,      bones::Head,      3.2f, GroupHead },
        { bones::Chest,     bones::Neck,      8.0f, GroupChest },
        { bones::Spine2,    bones::Chest,     8.5f, GroupChest },
        { bones::Pelvis,    bones::Spine2,    7.5f, GroupStomach },
        { bones::ArmUpperL, bones::ArmLowerL, 3.6f, GroupArms },
        { bones::ArmLowerL, bones::HandL,     3.2f, GroupArms },
        { bones::ArmUpperR, bones::ArmLowerR, 3.6f, GroupArms },
        { bones::ArmLowerR, bones::HandR,     3.2f, GroupArms },
        { bones::LegUpperL, bones::LegLowerL, 5.0f, GroupLegs },
        { bones::LegLowerL, bones::AnkleL,    4.2f, GroupLegs },
        { bones::LegUpperR, bones::LegLowerR, 5.0f, GroupLegs },
        { bones::LegLowerR, bones::AnkleR,    4.2f, GroupLegs },
    };

    int n = 0;
    for (const Def& d : defs) {
        if (n >= max)
            break;
        const Vector3 a = bone(d.a);
        const Vector3 b = bone(d.b);
        if (a.IsZero() || b.IsZero())
            continue;
        out[n++] = { a, b, d.r, d.group };
    }
    return n;
}

// Squared distance between a ray and a segment, plus how far along the ray the
// closest approach happens. This is the actual intersection test: a hit is
// when that distance drops below the capsule radius.
//
// Deliberately analytic rather than "is the player near the middle of the
// screen" - the ray comes from the real view angles, so it stays correct at
// any FOV, aspect ratio or resolution.
float RaySegmentDistSq(const Vector3& origin, const Vector3& dir,
                       const Vector3& a, const Vector3& b, float& rayT) {
    const Vector3 seg = b - a;
    const Vector3 diff = a - origin;

    const float segLenSq = seg.x * seg.x + seg.y * seg.y + seg.z * seg.z;
    const float dirDotSeg = dir.x * seg.x + dir.y * seg.y + dir.z * seg.z;
    const float dirDotDiff = dir.x * diff.x + dir.y * diff.y + dir.z * diff.z;
    const float segDotDiff = seg.x * diff.x + seg.y * diff.y + seg.z * diff.z;

    float s = 0.f;   // along the segment, clamped to it
    float t = 0.f;   // along the ray

    const float denom = segLenSq - dirDotSeg * dirDotSeg;
    if (segLenSq <= 1e-6f) {
        // Degenerate segment: a sphere. Closest point on the ray only.
        t = -dirDotDiff;
    } else if (std::fabs(denom) < 1e-6f) {
        // Parallel; any s works, take the segment start.
        t = -dirDotDiff;
    } else {
        s = (dirDotSeg * -dirDotDiff + segDotDiff * 1.f) / denom;
        s = s < 0.f ? 0.f : (s > 1.f ? 1.f : s);
        t = -dirDotDiff + dirDotSeg * s;
    }

    if (t < 0.f)
        t = 0.f;
    rayT = t;

    const Vector3 onRay{ origin.x + dir.x * t, origin.y + dir.y * t, origin.z + dir.z * t };
    const Vector3 onSeg{ a.x + seg.x * s, a.y + seg.y * s, a.z + seg.z * s };
    const Vector3 d = onRay - onSeg;
    return d.x * d.x + d.y * d.y + d.z * d.z;
}

Vector3 AngleToForward(float pitch, float yaw) {
    const float p = pitch / kRad2Deg;
    const float y = yaw / kRad2Deg;
    const float cp = std::cos(p);
    return { cp * std::cos(y), cp * std::sin(y), -std::sin(p) };
}

bool WeaponReady(const weapon::Active& gun, int tick, Stats& s) {
    s.weapon = (gun.ok && gun.info) ? gun.info->name : "none";
    s.clip = gun.clip;
    s.nextAttackTick = gun.nextAttackTick;

    if (!gun.ok || !gun.info)
        return false;

    // Knives, grenades and the C4 have no ballistics in the table and the
    // trigger has nothing sensible to do with them.
    if (gun.info->damage <= 0.f || gun.info->kind == weapon::Kind::Knife)
        return false;

    if (gun.reloading)
        return false;
    if (gun.clip == 0)
        return false;

    // The weapon publishes the tick it may next fire on. Comparing ticks keeps
    // the rate correct regardless of frame rate.
    if (tick > 0 && gun.nextAttackTick > tick)
        return false;

    return true;
}

// Which target we are currently sitting on, and since when - in GAME time, so
// the delay does not drift with frame rate and no thread ever sleeps.
uintptr_t g_heldPawn = 0;
float     g_heldSince = 0.f;

} // namespace

const char* GroupName(int group) {
    switch (group) {
    case GroupHead:    return "Head";
    case GroupChest:   return "Chest";
    case GroupStomach: return "Stomach";
    case GroupArms:    return "Arms";
    case GroupLegs:    return "Legs";
    default:           return "-";
    }
}

const char* RejectName(Reject r) {
    switch (r) {
    case Reject::None:            return "-";
    case Reject::Disabled:        return "DISABLED";
    case Reject::KeyUp:           return "KEY UP";
    case Reject::LocalDead:       return "LOCAL DEAD";
    case Reject::NoWeapon:        return "NO WEAPON";
    case Reject::WeaponNotReady:  return "WEAPON NOT READY";
    case Reject::NoTarget:        return "NO TARGET";
    case Reject::Teammate:        return "TEAMMATE";
    case Reject::InvalidHitgroup: return "INVALID HITGROUP";
    case Reject::NotVisible:      return "NOT VISIBLE";
    case Reject::Delay:           return "DELAY";
    case Reject::Fired:           return "FIRE";
    }
    return "?";
}

void Run(usercmd::UserCmd* cmd, uintptr_t localPawn) {
    Stats fresh;
    fresh.shotsFired = stats.shotsFired;

    const cfg::TriggerBot& tb = cfg::c.trigger;
    fresh.enabled = tb.enabled;

    if (!tb.enabled || !cmd || !g::client) {
        fresh.reject = Reject::Disabled;
        stats = fresh;
        g_heldPawn = 0;
        return;
    }

    fresh.keyHeld = tb.key == 0 || (GetAsyncKeyState(tb.key) & 0x8000) != 0;
    if (!fresh.keyHeld) {
        fresh.reject = Reject::KeyUp;
        stats = fresh;
        g_heldPawn = 0;
        return;
    }

    if (!mem::Valid(localPawn) || mem::Read<uint8_t>(localPawn + schema::m_lifeState) != 0) {
        fresh.reject = Reject::LocalDead;
        stats = fresh;
        g_heldPawn = 0;
        return;
    }

    // Game clock and tick, for the delay and the fire rate. Both come from the
    // vars the interface layer already validated.
    float now = 0.f;
    int tick = 0;
    if (const ifaces::GlobalVars* gv = ifaces::Globals()) {
        now = gv->curTime;
        tick = gv->tickCount;
    }
    fresh.tickCount = tick;

    const weapon::Active gun = weapon::Local(localPawn);
    if (!gun.ok || !gun.info || gun.info->damage <= 0.f ||
        gun.info->kind == weapon::Kind::Knife) {
        fresh.weapon = (gun.ok && gun.info) ? gun.info->name : "none";
        fresh.reject = Reject::NoWeapon;
        stats = fresh;
        g_heldPawn = 0;
        return;
    }

    fresh.weaponReady = WeaponReady(gun, tick, fresh);

    // ---- the ray ----------------------------------------------------------
    // Straight out of the player's own view angles, from the eye. Not the
    // screen centre, not a bounding box, not the nearest ESP entry.
    const Vector2 view = mem::Read<Vector2>(g::client + offsets::dwViewAngles);
    const Vector3 eye = sdk::GetAbsOrigin(localPawn) +
                        mem::Read<Vector3>(localPawn + schema::m_vecViewOffset);
    const Vector3 dir = AngleToForward(view.x, view.y);

    const uintptr_t entityList = mem::Read<uintptr_t>(g::client + offsets::dwEntityList);
    const int localTeam = mem::Read<uint8_t>(localPawn + schema::m_iTeamNum);

    uintptr_t hitPawn = 0;
    int  hitSlot = -1;
    int  hitGroup = -1;
    int  hitTeam = -1;
    float bestT = kRayLength;
    char hitName[32] = "-";

    if (mem::Valid(entityList) && bones::layout.calibrated) {
        for (int i = 1; i <= kMaxPlayers; ++i) {
            sdk::PlayerInfo p;
            if (!sdk::GetPlayer(entityList, i, p) || p.pawn == localPawn)
                continue;

            // Cheap rejection first: if the player's centre is far off the ray
            // there is no point building capsules for them.
            const Vector3 centre{ p.origin.x, p.origin.y, p.origin.z + 40.f };
            float rough = 0.f;
            if (RaySegmentDistSq(eye, dir, centre, centre, rough) > 80.f * 80.f)
                continue;

            Capsule caps[16];
            const int n = BuildCapsules(p.pawn, caps, 16);

            for (int c = 0; c < n; ++c) {
                float t = 0.f;
                const float distSq = RaySegmentDistSq(eye, dir, caps[c].a, caps[c].b, t);
                if (distSq > caps[c].radius * caps[c].radius)
                    continue;
                if (t <= 0.f || t >= bestT)
                    continue;

                bestT = t;
                hitPawn = p.pawn;
                hitSlot = i;
                hitGroup = caps[c].group;
                hitTeam = p.team;
                snprintf(hitName, sizeof(hitName), "%s", p.name.c_str());
            }
        }
    }

    if (!hitPawn) {
        fresh.reject = Reject::NoTarget;
        stats = fresh;
        g_heldPawn = 0;
        return;
    }

    fresh.targetFound = true;
    fresh.targetSlot = hitSlot;
    fresh.hitGroup = hitGroup;
    fresh.hitDistance = bestT;
    snprintf(fresh.targetName, sizeof(fresh.targetName), "%s", hitName);

    // ---- gates ------------------------------------------------------------
    fresh.teamValid = !tb.teamCheck || hitTeam != localTeam;
    if (!fresh.teamValid) {
        fresh.reject = Reject::Teammate;
        stats = fresh;
        g_heldPawn = 0;
        return;
    }

    if (hitGroup < 0 || hitGroup >= GroupCount || !tb.groups[hitGroup]) {
        fresh.reject = Reject::InvalidHitgroup;
        stats = fresh;
        g_heldPawn = 0;
        return;
    }

    if (tb.visibilityCheck) {
        fresh.visibilityKnown = visibility::Available();
        fresh.visible = fresh.visibilityKnown && visibility::IsVisible(hitPawn);
        if (!fresh.visible) {
            fresh.reject = Reject::NotVisible;
            stats = fresh;
            g_heldPawn = 0;
            return;
        }
    } else {
        fresh.visible = true;
    }

    // ---- delay ------------------------------------------------------------
    // A state machine on game time, not a Sleep and not a frame counter: the
    // same target has to stay under the ray for the configured window.
    if (hitPawn != g_heldPawn) {
        g_heldPawn = hitPawn;
        g_heldSince = now;
    }
    const float heldMs = (now - g_heldSince) * 1000.f;
    fresh.heldForMs = heldMs < 0.f ? 0.f : heldMs;
    fresh.delayElapsed = tb.delayMs <= 0 || fresh.heldForMs >= static_cast<float>(tb.delayMs);

    if (!fresh.delayElapsed) {
        fresh.reject = Reject::Delay;
        stats = fresh;
        return;
    }

    if (!fresh.weaponReady) {
        fresh.reject = Reject::WeaponNotReady;
        stats = fresh;
        return;
    }

    // ---- fire -------------------------------------------------------------
    // Straight into the command being built. No SendInput, no sleeping: the
    // attack bit rides the same command the server is about to receive, so the
    // rate follows the game's ticks rather than the frame rate.
    cmd->PressButton(usercmd::IN_ATTACK);
    ++fresh.shotsFired;
    fresh.reject = Reject::Fired;
    stats = fresh;
}

} // namespace triggerbot
