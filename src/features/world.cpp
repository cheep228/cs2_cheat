#include <Windows.h>

#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

#include "core/globals.h"
#include "core/log.h"
#include "features/world.h"
#include "sdk/entity.h"
#include "sdk/entitylist.h"
#include "sdk/interfaces.h"
#include "sdk/offsets.h"

#include "imgui.h"

namespace world {

Stats stats;

namespace {

// Smoke projectiles are ordinary entities, so they sit above the player slots.
// Scanning the whole 32k handle space every frame would cost more than the
// feature is worth; this range covers every projectile a live round produces.
constexpr int kFirstEntity = 1;
constexpr int kLastEntity  = 1024;

// How often the entity sweep runs. The cache is written to every frame - only
// the discovery is throttled, so a smoke that pops is suppressed within a
// couple of frames rather than a couple of seconds.
constexpr uint32_t kRescanMs = 200;

struct TrackedSmoke {
    uintptr_t entity   = 0;
    int       original = 0;   // m_nSmokeEffectTickBegin as the game set it
    bool      haveOriginal = false;
};

std::vector<TrackedSmoke> g_smokes;
uint32_t g_lastScan = 0;

// Mirrors the last state we acted on, so a transition can restore exactly
// once instead of fighting the game every frame.
bool g_smokeWasOn = false;
bool g_flashWasOn = false;

// CUtlSymbolLarge is a pointer to an interned string. Reading it costs three
// guarded reads, which is why discovery is throttled.
bool ClassNameIs(uintptr_t entity, const char* want) {
    const uintptr_t identity = mem::Read<uintptr_t>(entity + schema::m_pEntity);
    if (!mem::Valid(identity))
        return false;

    const uintptr_t namePtr = mem::Read<uintptr_t>(identity + schema::m_designerName);
    if (!mem::Valid(namePtr))
        return false;

    const std::string name = mem::ReadString(namePtr, 48);
    return name == want;
}

void ForgetSmokes(bool restoreFirst) {
    if (restoreFirst) {
        for (const TrackedSmoke& s : g_smokes) {
            if (s.haveOriginal && mem::Valid(s.entity))
                mem::Write<int32_t>(s.entity + schema::m_nSmokeEffectTickBegin, s.original);
        }
        if (!g_smokes.empty())
            DBG_I("world", "anti-smoke off: restored %d smoke(s)",
                  static_cast<int>(g_smokes.size()));
    }
    g_smokes.clear();
    stats.smokesTracked = 0;
    stats.smokesSuppressed = 0;
}

void RescanSmokes(uintptr_t entityList) {
    std::vector<TrackedSmoke> fresh;

    for (int i = kFirstEntity; i <= kLastEntity; ++i) {
        const uintptr_t ent = entlist::Get(entityList, i);
        if (!mem::Valid(ent))
            continue;
        if (!ClassNameIs(ent, "smokegrenade_projectile"))
            continue;

        TrackedSmoke t;
        t.entity = ent;

        // Carry the original forward if we already knew this entity, so a
        // rescan never records the zero we ourselves wrote as "the original".
        bool known = false;
        for (const TrackedSmoke& old : g_smokes) {
            if (old.entity == ent) {
                t.original = old.original;
                t.haveOriginal = old.haveOriginal;
                known = true;
                break;
            }
        }
        if (!known) {
            t.original = mem::Read<int32_t>(ent + schema::m_nSmokeEffectTickBegin);
            t.haveOriginal = true;
            DBG_I("world", "anti-smoke: new smoke @0x%llX, tickBegin=%d",
                  static_cast<unsigned long long>(ent), t.original);
        }

        fresh.push_back(t);
    }

    g_smokes.swap(fresh);
    stats.smokesTracked = static_cast<int>(g_smokes.size());
}

void RunAntiSmoke(uintptr_t entityList) {
    const bool on = cfg::c.world.antiSmoke;

    if (!on) {
        if (g_smokeWasOn)
            ForgetSmokes(/*restoreFirst=*/true);
        g_smokeWasOn = false;
        return;
    }
    g_smokeWasOn = true;

    const uint32_t now = GetTickCount();
    if (now - g_lastScan >= kRescanMs) {
        g_lastScan = now;
        RescanSmokes(entityList);
    }

    // A tick begin of 0 puts the effect's start far in the past, so the smoke
    // volume has already finished by the time it would be drawn. The entity,
    // its collision and the round logic are untouched.
    int written = 0;
    for (const TrackedSmoke& s : g_smokes) {
        if (!mem::Valid(s.entity))
            continue;
        if (mem::Write<int32_t>(s.entity + schema::m_nSmokeEffectTickBegin, 0))
            ++written;
    }
    stats.smokesSuppressed = written;
}

void RunAntiFlash(uintptr_t localPawn) {
    const bool on = cfg::c.world.antiFlash;

    stats.flashOverlay = mem::Read<float>(localPawn + schema::m_flFlashOverlayAlpha);

    if (!on) {
        if (g_flashWasOn) {
            // Hand the stock ceiling back; the game recomputes the overlay
            // from it on the next flashbang.
            mem::Write<float>(localPawn + schema::m_flFlashMaxAlpha, 255.f);
            DBG_I("world", "anti-flash off: m_flFlashMaxAlpha restored to 255");
        }
        g_flashWasOn = false;
        stats.flashClamped = false;
        return;
    }
    g_flashWasOn = true;

    const int pct = cfg::c.world.flashAmount < 0 ? 0
                  : (cfg::c.world.flashAmount > 100 ? 100 : cfg::c.world.flashAmount);
    const float ceiling = 255.f * (static_cast<float>(pct) / 100.f);

    // Two writes, because they do different jobs: the max alpha caps the next
    // flash, and the overlay alpha is what is on screen right now - clamping
    // only the first would leave an in-progress flash at full white.
    mem::Write<float>(localPawn + schema::m_flFlashMaxAlpha, ceiling);
    if (stats.flashOverlay > ceiling)
        mem::Write<float>(localPawn + schema::m_flFlashOverlayAlpha, ceiling);

    stats.flashClamped = true;
}

} // namespace

// ---------------------------------------------------------------------------
// C4
// ---------------------------------------------------------------------------

Bomb bomb;

namespace {

// The bomb changes state a handful of times a round, so discovery is throttled
// like the smoke sweep. The timer itself is recomputed every frame.
uint32_t g_lastBombScan = 0;

// One line per state change rather than per frame, so the console stays
// readable while still explaining a blank ESP.
bool g_loggedFound   = false;
bool g_loggedNoTime  = false;
bool g_loggedNoW2S   = false;

// Current GAME time. Taken from the global vars the interface layer already
// resolved and sanity-checked, not from a signature scan of our own and never
// from GetTickCount - a wall clock drifts against the server immediately.
bool CurTime(float& out) {
    const ifaces::GlobalVars* g = ifaces::Globals();
    if (!g)
        return false;
    if (!std::isfinite(g->curTime) || g->curTime <= 0.f)
        return false;
    out = g->curTime;
    return true;
}

bool FinitePos(const Vector3& v) {
    return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z) && !v.IsZero();
}

// A scene node is what carries the world transform. No node means the entity
// exists but has no position worth projecting.
bool HasSceneNode(uintptr_t entity) {
    return mem::Valid(mem::Read<uintptr_t>(entity + schema::m_pGameSceneNode));
}

// Walks the entity list once and sorts out both C4 states.
//
// Identification is by the entity's own designer name out of its identity
// struct - the same mechanism the smoke sweep uses - not by model path and not
// by interpreting every entity as a possible bomb.
void ScanBomb(uintptr_t entityList, uintptr_t localPawn) {
    Bomb fresh;
    const Vector3 localOrigin = mem::Valid(localPawn) ? sdk::GetAbsOrigin(localPawn)
                                                      : Vector3{};

    for (int i = kFirstEntity; i <= kLastEntity; ++i) {
        const uintptr_t ent = entlist::Get(entityList, i);
        if (!mem::Valid(ent))
            continue;
        if (!HasSceneNode(ent))
            continue;

        // ---- planted ------------------------------------------------------
        if (!fresh.found && ClassNameIs(ent, "planted_c4")) {
            const Vector3 pos = sdk::GetAbsOrigin(ent);
            if (!FinitePos(pos))
                continue;

            fresh.found   = true;
            fresh.entity  = ent;
            fresh.origin  = pos;
            fresh.ticking = mem::Read<bool>(ent + schema::m_bBombTicking);
            fresh.defused = mem::Read<bool>(ent + schema::m_bBombDefused);
            fresh.site    = mem::Read<int32_t>(ent + schema::m_nBombSite);
            fresh.beingDefused = mem::Read<bool>(ent + schema::m_bBeingDefused);
            continue;
        }

        // ---- dropped ------------------------------------------------------
        if (!fresh.droppedFound && ClassNameIs(ent, "weapon_c4")) {
            // A bomb in someone's inventory has an owner. Drawing that at the
            // weapon entity's position would put a "C4" marker at a stale or
            // meaningless spot, so only ownerless ones count as dropped.
            const uint32_t owner = mem::Read<uint32_t>(ent + schema::m_hOwnerEntity);
            if (owner != 0xFFFFFFFF)
                continue;

            const Vector3 pos = sdk::GetAbsOrigin(ent);
            if (!FinitePos(pos))
                continue;

            fresh.droppedFound  = true;
            fresh.droppedEntity = ent;
            fresh.droppedOrigin = pos;
            if (!localOrigin.IsZero())
                fresh.droppedMeters = (pos - localOrigin).Length() / 52.5f;
            continue;
        }

        if (fresh.found && fresh.droppedFound)
            break;
    }

    // Carry over what the timer computed last frame; ScanBomb only discovers.
    fresh.timerValid      = bomb.timerValid;
    fresh.remaining       = bomb.remaining;
    fresh.defuseValid     = bomb.defuseValid;
    fresh.defuseRemaining = bomb.defuseRemaining;
    bomb = fresh;

    if (bomb.found && !g_loggedFound) {
        g_loggedFound = true;
        DBG_I("bomb", "C_PlantedC4 found @0x%llX, pos (%.0f %.0f %.0f), ticking=%s site=%d",
              static_cast<unsigned long long>(bomb.entity),
              bomb.origin.x, bomb.origin.y, bomb.origin.z,
              bomb.ticking ? "yes" : "no", bomb.site);
    } else if (!bomb.found) {
        g_loggedFound = false;
    }
}

// Both countdowns, from the game's own clock.
//
//   remaining = blow_time - curtime
//   defuse    = defuse_countdown - curtime
//
// Both stored values are absolute game times, which is why nothing here starts
// a stopwatch of its own.
void UpdateBombTimer() {
    bomb.timerValid = false;
    bomb.defuseValid = false;
    bomb.remaining = 0.f;
    bomb.defuseRemaining = 0.f;
    bomb.defuseInTime = false;

    if (!bomb.found || !bomb.ticking || bomb.defused)
        return;

    float now = 0.f;
    if (!CurTime(now)) {
        if (!g_loggedNoTime) {
            g_loggedNoTime = true;
            DBG_W("bomb", "no trusted global vars - timer hidden rather than guessed");
        }
        return;
    }
    g_loggedNoTime = false;
    bomb.lastCurTime = now;

    const float blow = mem::Read<float>(bomb.entity + schema::m_flC4Blow);
    bomb.lastBlowTime = blow;
    if (!std::isfinite(blow) || blow <= 0.f)
        return;

    const float remaining = blow - now;

    // A C4 fuse is 40 s. Anything outside that window means a field or the
    // clock is wrong for this build, and a wrong number is worse than none.
    if (!std::isfinite(remaining) || remaining < -5.f || remaining > 60.f) {
        DBG_EVERY(5000, dbg::Level::Warn, "bomb",
                  "blow=%.1f curtime=%.1f gives %.1f s - field or clock is wrong",
                  blow, now, remaining);
        return;
    }

    bomb.timerValid = true;
    bomb.remaining = remaining < 0.f ? 0.f : remaining;

    if (bomb.beingDefused) {
        const float until = mem::Read<float>(bomb.entity + schema::m_flDefuseCountDown);
        const float left = until - now;
        if (std::isfinite(left) && left > -1.f && left < 15.f) {
            bomb.defuseValid = true;
            bomb.defuseRemaining = left < 0.f ? 0.f : left;
            bomb.defuseInTime = bomb.defuseRemaining <= bomb.remaining;
        }
    }
}

} // namespace

void Draw(ImDrawList* draw) {
    const cfg::World& w = cfg::c.world;
    if (!draw || (!w.bombEsp && !w.bombTimer))
        return;

    const Matrix4x4 view = mem::Read<Matrix4x4>(g::client + offsets::dwViewMatrix);
    const ImU32 color = ImGui::ColorConvertFloat4ToU32(w.bombColor);

    auto label = [&](const ImVec2& at, const char* text, ImU32 col) {
        const ImVec2 size = ImGui::CalcTextSize(text);
        const ImVec2 pos(at.x - size.x * 0.5f, at.y);
        draw->AddText(ImVec2(pos.x + 1.f, pos.y + 1.f), IM_COL32(0, 0, 0, 210), text);
        draw->AddText(pos, col, text);
    };

    // ---- dropped ----------------------------------------------------------
    if (w.bombEsp && bomb.droppedFound) {
        Vector2 screen;
        if (WorldToScreen(view, bomb.droppedOrigin, screen, g::screenW, g::screenH)) {
            const ImVec2 at(screen.x, screen.y);
            constexpr float r = 4.f;
            const ImVec2 pts[4] = {
                ImVec2(at.x, at.y - r), ImVec2(at.x + r, at.y),
                ImVec2(at.x, at.y + r), ImVec2(at.x - r, at.y),
            };
            draw->AddConvexPolyFilled(pts, 4, color);
            label(ImVec2(at.x, at.y + r + 2.f), "C4", color);

            char dist[24];
            snprintf(dist, sizeof(dist), "%.0fm", bomb.droppedMeters);
            label(ImVec2(at.x, at.y + r + 2.f + ImGui::GetTextLineHeight()), dist, color);
        }
    }

    // ---- planted ----------------------------------------------------------
    if (!bomb.found)
        return;

    Vector2 screen;
    bomb.w2sOk = WorldToScreen(view, bomb.origin, screen, g::screenW, g::screenH);
    if (!bomb.w2sOk) {
        if (!g_loggedNoW2S) {
            g_loggedNoW2S = true;
            DBG_I("bomb", "planted bomb found but behind the camera - nothing to draw");
        }
        return;
    }
    g_loggedNoW2S = false;

    const ImVec2 at(screen.x, screen.y);
    float y = at.y;

    if (w.bombEsp) {
        constexpr float r = 5.f;
        const ImVec2 pts[4] = {
            ImVec2(at.x, at.y - r), ImVec2(at.x + r, at.y),
            ImVec2(at.x, at.y + r), ImVec2(at.x - r, at.y),
        };
        draw->AddConvexPolyFilled(pts, 4, color);
        draw->AddPolyline(pts, 4, IM_COL32(0, 0, 0, 200), ImDrawFlags_Closed, 1.4f);

        char text[32];
        if (bomb.defused)
            snprintf(text, sizeof(text), "BOMB DEFUSED");
        else if (bomb.site == 0 || bomb.site == 1)
            snprintf(text, sizeof(text), "BOMB %c", bomb.site == 0 ? 'A' : 'B');
        else
            snprintf(text, sizeof(text), "BOMB");

        y = at.y + r + 2.f;
        label(ImVec2(at.x, y), text, bomb.defused ? IM_COL32(90, 200, 110, 255) : color);
        y += ImGui::GetTextLineHeight();
    }

    if (!w.bombTimer || bomb.defused)
        return;

    if (bomb.timerValid) {
        char text[32];
        snprintf(text, sizeof(text), "%.1fs", bomb.remaining);
        // Red once a full defuse no longer fits in what is left.
        const ImU32 col = bomb.remaining <= 10.f ? IM_COL32(235, 70, 70, 255) : color;
        label(ImVec2(at.x, y), text, col);
        y += ImGui::GetTextLineHeight();
    }

    if (bomb.defuseValid) {
        char text[40];
        snprintf(text, sizeof(text), "DEFUSE: %.1fs", bomb.defuseRemaining);
        // Green when the defuse lands before the bomb does, red when it does not.
        const ImU32 col = bomb.defuseInTime ? IM_COL32(90, 200, 110, 255)
                                            : IM_COL32(235, 70, 70, 255);
        label(ImVec2(at.x, y), text, col);
    }
}

void Run() {
    stats.localPawnOk = false;

    if (!g::client)
        return;

    const uintptr_t localPawn = mem::Read<uintptr_t>(g::client + offsets::dwLocalPlayerPawn);
    if (mem::Valid(localPawn)) {
        stats.localPawnOk = true;
        RunAntiFlash(localPawn);
    }

    const uintptr_t entityList = mem::Read<uintptr_t>(g::client + offsets::dwEntityList);
    if (mem::Valid(entityList) && entlist::layout.calibrated) {
        RunAntiSmoke(entityList);

        // The bomb is planted once a round, so discovery is throttled the same
        // way the smoke sweep is; only the countdown updates every frame.
        if (cfg::c.world.bombEsp || cfg::c.world.bombTimer) {
            const uint32_t now = GetTickCount();
            if (now - g_lastBombScan >= kRescanMs || (!bomb.found && !bomb.droppedFound)) {
                g_lastBombScan = now;
                ScanBomb(entityList, localPawn);
            }
            UpdateBombTimer();
        } else if (bomb.found) {
            bomb = Bomb{};
        }
    }
}

void Restore() {
    ForgetSmokes(/*restoreFirst=*/true);
    g_smokeWasOn = false;

    if (g_flashWasOn && g::client) {
        const uintptr_t localPawn =
            mem::Read<uintptr_t>(g::client + offsets::dwLocalPlayerPawn);
        if (mem::Valid(localPawn))
            mem::Write<float>(localPawn + schema::m_flFlashMaxAlpha, 255.f);
    }
    g_flashWasOn = false;
}

} // namespace world
