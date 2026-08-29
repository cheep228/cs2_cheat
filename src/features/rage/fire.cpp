#include <Windows.h>

#include "features/rage/fire.h"
#include "sdk/entity.h"
#include "sdk/offsets.h"

namespace rage {
namespace fire {
namespace {

uint32_t g_lastShot  = 0;
uint32_t g_lastScope = 0;

void MouseClick(DWORD downFlag, DWORD upFlag) {
    INPUT in[2]{};
    in[0].type = INPUT_MOUSE;
    in[0].mi.dwFlags = downFlag;
    in[1].type = INPUT_MOUSE;
    in[1].mi.dwFlags = upFlag;
    SendInput(2, in, sizeof(INPUT));
}

// Scancodes for W A S D sent as key-UP only. This cannot press anything; it can
// only let go of what the player is already holding, which is the whole point.
void ReleaseMovementKeys() {
    static const WORD kScan[] = { 0x11, 0x1E, 0x1F, 0x20 };
    INPUT in[4]{};
    for (int i = 0; i < 4; ++i) {
        in[i].type = INPUT_KEYBOARD;
        in[i].ki.wScan = kScan[i];
        in[i].ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
    }
    SendInput(4, in, sizeof(INPUT));
}

// Shortest sensible gap between synthetic clicks, by weapon class.
int CycleFor(const weapon::Active& gun) {
    if (!gun.info)
        return 120;

    switch (gun.info->kind) {
    case weapon::Kind::Sniper:  return 250;   // bolt / heavy recoil reset
    case weapon::Kind::Shotgun: return 220;
    case weapon::Kind::Pistol:  return 110;
    default:                    return 90;
    }
}

} // namespace

float StopSpeedFor(const weapon::Active& gun) {
    if (!gun.info)
        return 80.f;

    switch (gun.info->kind) {
    // A moving sniper does not have a hitchance, it has a story. Anything
    // above a crawl and the shot is not worth taking.
    case weapon::Kind::Sniper:     return 6.f;
    case weapon::Kind::Rifle:      return 80.f;
    case weapon::Kind::Machinegun: return 60.f;
    case weapon::Kind::Smg:        return 95.f;
    case weapon::Kind::Pistol:     return 105.f;
    case weapon::Kind::Shotgun:    return 130.f;
    default:                       return 80.f;
    }
}

bool AutoStop(int mode, const weapon::Active& gun, float speed) {
    if (mode == cfg::AutoStopOff)
        return true;

    const float limit = StopSpeedFor(gun);
    if (speed <= limit)
        return true;

    if (mode == cfg::AutoStopFull)
        ReleaseMovementKeys();

    return false;
}

bool NeedsScope(const weapon::Active& gun) {
    return gun.ok && gun.info && gun.info->scopeable && !gun.scoped;
}

void ToggleScope() {
    const uint32_t now = GetTickCount();
    if (now - g_lastScope < 400)
        return;
    g_lastScope = now;
    MouseClick(MOUSEEVENTF_RIGHTDOWN, MOUSEEVENTF_RIGHTUP);
}

bool Shoot(const weapon::Active& gun) {
    const uint32_t now = GetTickCount();
    if (now - g_lastShot < static_cast<uint32_t>(CycleFor(gun)))
        return false;
    g_lastShot = now;
    MouseClick(MOUSEEVENTF_LEFTDOWN, MOUSEEVENTF_LEFTUP);
    return true;
}

void WriteViewAngles(const Angle& angles) {
    const Vector2 out{ ClampPitch(angles.pitch), NormalizeYaw(angles.yaw) };
    mem::Write<Vector2>(g::client + offsets::dwViewAngles, out);
}

void Reset() {
    g_lastShot = 0;
    g_lastScope = 0;
}

} // namespace fire
} // namespace rage
