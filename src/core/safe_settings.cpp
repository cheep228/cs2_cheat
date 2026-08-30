#include "core/globals.h"
#include "core/log.h"
#include "core/safe_settings.h"

namespace safeset {
namespace {

// Mirrors the state we last acted on. A config load can flip cfg::c.safe
// underneath us, and comparing against this is what turns that into a proper
// transition instead of a silent half-applied state.
bool g_applied = false;

void Engage() {
    cfg::SafeSettings& s = cfg::c.safe;
    cfg::Aimbot& a = cfg::c.aim;

    // Stash once. Engaging twice without a release in between must not
    // overwrite the user's originals with the clamped values.
    if (!s.saved) {
        s.prevFov = a.fov;
        s.prevSmooth = a.smooth;
        s.prevVisibilityCheck = a.visibilityCheck;
        s.saved = true;
    }

    if (a.fov > cfg::SafeSettings::kMaxFov)
        a.fov = cfg::SafeSettings::kMaxFov;
    if (a.smooth < cfg::SafeSettings::kMinSmooth)
        a.smooth = cfg::SafeSettings::kMinSmooth;
    a.visibilityCheck = true;

    DBG_I("safe", "engaged: fov %.1f -> %.1f, smooth %.1f -> %.1f, visibility forced on",
          s.prevFov, a.fov, s.prevSmooth, a.smooth);
}

void Release() {
    cfg::SafeSettings& s = cfg::c.safe;
    cfg::Aimbot& a = cfg::c.aim;

    if (!s.saved)
        return;

    a.fov = s.prevFov;
    a.smooth = s.prevSmooth;
    a.visibilityCheck = s.prevVisibilityCheck;
    s.saved = false;

    DBG_I("safe", "released: fov %.1f, smooth %.1f, visibility %s restored",
          a.fov, a.smooth, a.visibilityCheck ? "on" : "off");
}

// Cheap per-frame guard. It writes only when something is genuinely out of
// bounds, which after Engage() and with the UI bounded can only happen when a
// config slot is loaded while the guard rail is on.
void HoldTheLine() {
    cfg::Aimbot& a = cfg::c.aim;

    if (a.fov > cfg::SafeSettings::kMaxFov) {
        a.fov = cfg::SafeSettings::kMaxFov;
        DBG_W("safe", "fov exceeded the safe maximum and was clamped");
    }
    if (a.smooth < cfg::SafeSettings::kMinSmooth) {
        a.smooth = cfg::SafeSettings::kMinSmooth;
        DBG_W("safe", "smooth below the safe minimum and was raised");
    }
    if (!a.visibilityCheck) {
        a.visibilityCheck = true;
        DBG_W("safe", "visibility check switched off and was forced back on");
    }
}

} // namespace

bool Active() {
    return cfg::c.safe.enabled;
}

void Sync() {
    const bool want = cfg::c.safe.enabled;

    if (want && !g_applied) {
        Engage();
        g_applied = true;
        return;
    }

    if (!want && g_applied) {
        Release();
        g_applied = false;
        return;
    }

    if (want)
        HoldTheLine();
}

} // namespace safeset
