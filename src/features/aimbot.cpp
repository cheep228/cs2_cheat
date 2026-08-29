#include <Windows.h>
#include <cmath>

#include "core/globals.h"
#include "core/log.h"
#include "features/aimbot.h"
#include "sdk/bones.h"
#include "sdk/entity.h"
#include "sdk/entitylist.h"
#include "sdk/math.h"
#include "sdk/offsets.h"

namespace {

constexpr int kMaxPlayers = 64;

// Eye position: the pawn origin plus its view offset.
Vector3 EyePosition(uintptr_t pawn) {
    const Vector3 origin = sdk::GetAbsOrigin(pawn);
    const Vector3 offset = mem::Read<Vector3>(pawn + schema::m_vecViewOffset);
    return origin + offset;
}

// Moves `current` a fraction of the way to `target`. smooth == 1 snaps.
Angle Smooth(const Angle& current, const Angle& target, float smooth) {
    if (smooth <= 1.f)
        return target;

    Angle out;
    out.pitch = current.pitch + (target.pitch - current.pitch) / smooth;
    out.yaw = current.yaw + NormalizeYaw(target.yaw - current.yaw) / smooth;
    out.roll = 0.f;

    out.pitch = ClampPitch(out.pitch);
    out.yaw = NormalizeYaw(out.yaw);
    return out;
}

} // namespace

namespace aimbot {

State state;

const char* const kPartNames[] = {
    "Pelvis", "Head", "Torso", "Neck",
    "Left arm", "Right arm", "Left leg", "Right leg",
};

int BoneForPart(int part) {
    switch (part) {
    case cfg::PartPelvis: return bones::Pelvis;
    case cfg::PartHead:   return bones::Head;
    case cfg::PartTorso:  return bones::Chest;
    case cfg::PartNeck:   return bones::Neck;
    case cfg::PartArmL:   return bones::ArmUpperL;
    case cfg::PartArmR:   return bones::ArmUpperR;
    case cfg::PartLegL:   return bones::LegUpperL;
    case cfg::PartLegR:   return bones::LegUpperR;
    default:              return -1;
    }
}

void Run() {
    State fresh;
    const cfg::Aimbot& a = cfg::c.aim;

    if (!cfg::c.masterSwitch || !a.enabled || !g::client) {
        state = fresh;
        return;
    }

    fresh.active = (GetAsyncKeyState(a.key) & 0x8000) != 0;
    if (!fresh.active) {
        state = fresh;
        return;
    }

    if (!bones::layout.calibrated) {
        DBG_EVERY(5000, dbg::Level::Warn, "aim", "bones not calibrated - aimbot idle");
        state = fresh;
        return;
    }

    const uintptr_t entityList = mem::Read<uintptr_t>(g::client + offsets::dwEntityList);
    const uintptr_t localPawn = mem::Read<uintptr_t>(g::client + offsets::dwLocalPlayerPawn);
    if (!mem::Valid(entityList) || !mem::Valid(localPawn)) {
        state = fresh;
        return;
    }

    const int localTeam = mem::Read<uint8_t>(localPawn + schema::m_iTeamNum);
    const Vector3 eye = EyePosition(localPawn);

    const Angle current = [&] {
        const Vector2 raw = mem::Read<Vector2>(g::client + offsets::dwViewAngles);
        Angle v;
        v.pitch = raw.x;
        v.yaw = raw.y;
        return v;
    }();

    Angle best{};
    float bestFov = a.fov;
    bool found = false;

    for (int i = 1; i <= kMaxPlayers; ++i) {
        sdk::PlayerInfo p;
        if (!sdk::GetPlayer(entityList, i, p))
            continue;
        if (p.pawn == localPawn)
            continue;
        if (a.teamCheck && p.team == localTeam)
            continue;
        if (p.isBot ? !a.onBots : !a.onPlayers)
            continue;

        ++fresh.candidates;

        for (int part = 0; part < cfg::PartCount; ++part) {
            if (!a.parts[part])
                continue;

            const int bone = BoneForPart(part);
            const Vector3 point = bones::Position(p.pawn, bone);
            if (point.IsZero())
                continue;

            const Angle want = CalcAngle(eye, point);
            const float fov = AngleDelta(want, current);
            if (fov >= bestFov)
                continue;

            bestFov = fov;
            best = want;
            found = true;
            fresh.hasTarget = true;
            fresh.targetSlot = i;
            fresh.targetPart = part;
            fresh.targetFov = fov;

            const Matrix4x4 view = mem::Read<Matrix4x4>(g::client + offsets::dwViewMatrix);
            WorldToScreen(view, point, fresh.targetScreen, g::screenW, g::screenH);
        }
    }

    if (found) {
        const Angle applied = Smooth(current, best, a.smooth);
        Vector2 out{ applied.pitch, applied.yaw };
        mem::Write<Vector2>(g::client + offsets::dwViewAngles, out);
    }

    state = fresh;
}

void Draw(ImDrawList* d, int screenW, int screenH) {
    const cfg::Aimbot& a = cfg::c.aim;
    if (!cfg::c.masterSwitch || !a.enabled)
        return;

    const ImVec2 centre(screenW * 0.5f, screenH * 0.5f);

    if (a.drawFov) {
        // The FOV cone is angular; at the screen centre it maps to roughly
        // (screenH / vertical fov) pixels per degree. 90 deg horizontal is the
        // CS2 default, so this is an approximation good enough to aim by.
        const float pixelsPerDegree = screenH / 60.f;
        const float radius = a.fov * pixelsPerDegree;
        d->AddCircle(centre, radius, ImGui::ColorConvertFloat4ToU32(a.fovColor), 64, 1.4f);
    }

    if (a.drawTarget && state.hasTarget) {
        const ImU32 col = ImGui::ColorConvertFloat4ToU32(a.targetColor);
        d->AddLine(centre, ImVec2(state.targetScreen.x, state.targetScreen.y), col, 1.2f);
        d->AddCircleFilled(ImVec2(state.targetScreen.x, state.targetScreen.y), 3.f, col);
    }
}

} // namespace aimbot
