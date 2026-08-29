#include <Windows.h>
#include <cstdio>

#include "core/globals.h"
#include "core/log.h"
#include "features/chams.h"
#include "features/esp.h"
#include "features/skeleton.h"
#include "sdk/bones.h"
#include "sdk/entity.h"
#include "sdk/entitylist.h"
#include "sdk/gameinfo.h"
#include "sdk/math.h"
#include "sdk/offsets.h"
#include "sdk/resolve.h"

namespace {

constexpr float kStandingHeight = 72.f;  // Source units from feet to top of head
constexpr float kUnitsPerMeter  = 39.37f;
constexpr int   kMaxPlayers     = 64;

struct Box {
    float x, y, w, h;
};

ImU32 ToU32(const ImVec4& c) { return ImGui::ColorConvertFloat4ToU32(c); }

// printf-friendly widening for pointer values.
unsigned long long U(uintptr_t v) { return static_cast<unsigned long long>(v); }

void DrawOutlinedText(ImDrawList* d, ImVec2 pos, ImU32 color, const char* text) {
    const ImU32 shadow = IM_COL32(0, 0, 0, 200);
    d->AddText(ImVec2(pos.x + 1, pos.y + 1), shadow, text);
    d->AddText(pos, color, text);
}

void DrawCenteredText(ImDrawList* d, float cx, float y, ImU32 color, const char* text) {
    const ImVec2 size = ImGui::CalcTextSize(text);
    DrawOutlinedText(d, ImVec2(cx - size.x * 0.5f, y), color, text);
}

void DrawBox(ImDrawList* d, const Box& b, ImU32 color, int type) {
    const ImVec2 tl(b.x, b.y);
    const ImVec2 br(b.x + b.w, b.y + b.h);
    const ImU32 outline = IM_COL32(0, 0, 0, 190);

    switch (type) {
    case cfg::Box2D:
        d->AddRect(ImVec2(tl.x - 1, tl.y - 1), ImVec2(br.x + 1, br.y + 1), outline, 0.f, 0, 3.f);
        d->AddRect(tl, br, color, 0.f, 0, 1.5f);
        break;

    case cfg::BoxFilled:
        d->AddRectFilled(tl, br, (color & 0x00FFFFFF) | 0x30000000);
        d->AddRect(tl, br, color, 0.f, 0, 1.5f);
        break;

    case cfg::BoxCorners:
    default: {
        const float lx = b.w * 0.28f;
        const float ly = b.h * 0.18f;
        auto corner = [&](ImVec2 a, ImVec2 hEnd, ImVec2 vEnd) {
            d->AddLine(a, hEnd, outline, 3.f);
            d->AddLine(a, vEnd, outline, 3.f);
            d->AddLine(a, hEnd, color, 1.5f);
            d->AddLine(a, vEnd, color, 1.5f);
        };
        corner(tl, ImVec2(tl.x + lx, tl.y), ImVec2(tl.x, tl.y + ly));
        corner(ImVec2(br.x, tl.y), ImVec2(br.x - lx, tl.y), ImVec2(br.x, tl.y + ly));
        corner(ImVec2(tl.x, br.y), ImVec2(tl.x + lx, br.y), ImVec2(tl.x, br.y - ly));
        corner(br, ImVec2(br.x - lx, br.y), ImVec2(br.x, br.y - ly));
        break;
    }
    }
}

void DrawHealthBar(ImDrawList* d, const Box& b, int health) {
    const float frac = (health < 0 ? 0 : (health > 100 ? 100 : health)) / 100.f;

    const float x  = b.x - 7.f;
    const float y  = b.y;
    const float h  = b.h;
    const float w  = 3.f;

    d->AddRectFilled(ImVec2(x - 1, y - 1), ImVec2(x + w + 1, y + h + 1), IM_COL32(0, 0, 0, 200));

    const ImU32 hpColor = IM_COL32(static_cast<int>(255 * (1.f - frac)),
                                   static_cast<int>(255 * frac), 40, 255);
    d->AddRectFilled(ImVec2(x, y + h * (1.f - frac)), ImVec2(x + w, y + h), hpColor);
}

// Logs only when something actually changes, plus a slow heartbeat, so the
// console stays readable at 300 fps.
void ReportState(const esp::Stats& s) {
    const int signature = (s.entityListOk ? 1 : 0) | (s.localPawnOk ? 2 : 0) |
                          (s.viewMatrixOk ? 4 : 0);

    static int lastSignature = -1;
    if (signature != lastSignature) {
        lastSignature = signature;
        const dbg::Level lvl = signature == 7 ? dbg::Level::Ok : dbg::Level::Warn;
        dbg::Print(lvl, "esp", "entityList=%s  localPawn=%s  viewMatrix=%s",
                   s.entityListOk ? "ok" : "FAIL",
                   s.localPawnOk ? "ok" : "FAIL",
                   s.viewMatrixOk ? "ok" : "FAIL");

        if (!s.entityListOk)
            DBG_E("esp", "dwEntityList is wrong for this build - run update_offsets.py");
        else if (!s.localPawnOk)
            DBG_W("esp", "no local pawn - not spawned in yet, or dwLocalPlayerPawn is stale");
        else if (!s.viewMatrixOk)
            DBG_E("esp", "view matrix is all zero - dwViewMatrix is stale");
    }

    static int lastPlayerCount = -1;
    if (s.entityListOk && s.slotsScanned != lastPlayerCount) {
        lastPlayerCount = s.slotsScanned;
        DBG_I("esp", "live players: %d (humans %d / bots %d), drawn %d",
              s.slotsScanned, s.humans, s.bots, s.drawn);
    }

    // Report the faults accumulated since the last line, not since the last
    // frame — the rate is what says whether reads are failing continuously.
    static unsigned long lastReported = 0;
    static DWORD lastReportTick = 0;
    const DWORD now = GetTickCount();
    if (mem::faults > lastReported && now - lastReportTick >= 3000) {
        const float elapsed = lastReportTick ? (now - lastReportTick) / 1000.f : 0.f;
        DBG_W("mem", "%lu faulting reads (+%lu in %.1f s) - offsets stale for this build",
              mem::faults, mem::faults - lastReported, elapsed);
        lastReported = mem::faults;
        lastReportTick = now;
    }

    if (s.entityListOk && s.slotsScanned == 0)
        DBG_EVERY(5000, dbg::Level::Warn, "esp",
                  "entity list reads but holds no live players - schema offsets stale?");
    else if (s.slotsScanned > 0 && s.drawn == 0)
        DBG_EVERY(5000, dbg::Level::Warn, "esp",
                  "%d players found, 0 drawn - team check (local team %d), max distance "
                  "%.0f m, or a disabled group is filtering them",
                  s.slotsScanned, s.localTeam, cfg::c.maxDistance);

    if (cfg::c.logVerbose)
        DBG_EVERY(1000, dbg::Level::Trace, "esp",
                  "frame: scanned %d, humans %d, bots %d, drawn %d, faults %lu",
                  s.slotsScanned, s.humans, s.bots, s.drawn, mem::faults);
}

} // namespace

namespace esp {

void Render(ImDrawList* draw) {
    stats = Stats{};

    if (!cfg::c.espEnabled || !g::client || g::screenW <= 0) {
        DBG_EVERY(5000, dbg::Level::Trace, "esp", "idle: esp=%d client=0x%llX screen=%dx%d",
                  cfg::c.espEnabled ? 1 : 0,
                  static_cast<unsigned long long>(g::client), g::screenW, g::screenH);
        return;
    }

    const uintptr_t entityList = mem::Read<uintptr_t>(g::client + offsets::dwEntityList);
    stats.entityListOk = mem::Valid(entityList);
    DBG_ONCE(dbg::Level::Info, "esp", "entity list ptr = 0x%llX (client+0x%llX)",
             static_cast<unsigned long long>(entityList),
             static_cast<unsigned long long>(offsets::dwEntityList));
    if (!stats.entityListOk) {
        ReportState(stats);
        return;
    }

    const uintptr_t localPawn = mem::Read<uintptr_t>(g::client + offsets::dwLocalPlayerPawn);
    stats.localPawnOk = mem::Valid(localPawn);
    if (!stats.localPawnOk) {
        ReportState(stats);
        return;
    }

    // Needs a live local player, so it can only happen once in a match; retried
    // each frame until it succeeds, then it costs nothing.
    if (!entlist::layout.calibrated) {
        const uintptr_t localCtrl =
            mem::Read<uintptr_t>(g::client + offsets::dwLocalPlayerController);
        entlist::Calibrate(entityList, localCtrl, localPawn);
    }

    const int localTeam = mem::Read<uint8_t>(localPawn + schema::m_iTeamNum);
    stats.localTeam = localTeam;
    const Vector3 localOrigin = sdk::GetAbsOrigin(localPawn);

    // Same trick as the entity list: probe the bone array against a pawn whose
    // origin we already trust.
    if (!bones::layout.calibrated)
        bones::Calibrate(localPawn, localOrigin);

    const uint32_t nowMs = GetTickCount();
    chams::lastApplied = 0;
    const Matrix4x4 viewMatrix = mem::Read<Matrix4x4>(g::client + offsets::dwViewMatrix);
    // A live matrix is never all-zero; a zero row means the offset is wrong.
    stats.viewMatrixOk = viewMatrix.m[3][0] != 0.f || viewMatrix.m[3][1] != 0.f ||
                         viewMatrix.m[3][2] != 0.f || viewMatrix.m[3][3] != 0.f;

    for (int i = 1; i <= kMaxPlayers; ++i) {
        sdk::PlayerInfo p;
        if (!sdk::GetPlayer(entityList, i, p))
            continue;
        if (p.pawn == localPawn)
            continue;

        ++stats.slotsScanned;
        if (p.isBot) ++stats.bots; else ++stats.humans;

        const cfg::EspGroup& grp = p.isBot ? cfg::c.bots : cfg::c.players;
        if (!grp.enabled)
            continue;
        if (cfg::c.teamCheck && p.team == localTeam)
            continue;

        const float meters = (p.origin - localOrigin).Length() / kUnitsPerMeter;
        if (meters > cfg::c.maxDistance)
            continue;

        // Glow/chams are writes into the game, independent of whether the
        // player ends up on screen.
        chams::Apply(p.pawn, grp);

        // History has to be fed every frame, or the delayed pose has gaps.
        if (grp.aimHistory)
            skeleton::Record(i, p.pawn, nowMs);

        Vector2 foot{}, head{};
        const Vector3 headPos(p.origin.x, p.origin.y, p.origin.z + kStandingHeight);
        if (!WorldToScreen(viewMatrix, p.origin, foot, g::screenW, g::screenH) ||
            !WorldToScreen(viewMatrix, headPos, head, g::screenW, g::screenH))
            continue;

        const float boxH = foot.y - head.y;
        if (boxH <= 1.f)
            continue;
        const float boxW = boxH * 0.42f;

        const Box box{ head.x - boxW * 0.5f, head.y, boxW, boxH };
        const ImU32 color = ToU32(grp.color);
        ++stats.drawn;

        if (grp.snapline)
            draw->AddLine(ImVec2(g::screenW * 0.5f, static_cast<float>(g::screenH)),
                          ImVec2(foot.x, foot.y), color, 1.2f);

        // The trailing ghost goes first so the live skeleton draws over it.
        if (grp.aimHistory) {
            skeleton::Pose past;
            if (skeleton::Delayed(i, nowMs, cfg::c.aimHistoryDelayMs, past))
                skeleton::Draw(draw, past, viewMatrix, g::screenW, g::screenH,
                               ToU32(grp.aimHistoryColor), cfg::c.skeletonThickness);
        }

        if (grp.skeleton) {
            skeleton::Pose live;
            if (skeleton::Live(p.pawn, live))
                skeleton::Draw(draw, live, viewMatrix, g::screenW, g::screenH,
                               ToU32(grp.skeletonColor), cfg::c.skeletonThickness);
        }

        if (grp.box)
            DrawBox(draw, box, color, grp.boxType);

        if (grp.health)
            DrawHealthBar(draw, box, p.health);

        if (grp.name)
            DrawCenteredText(draw, head.x, box.y - 15.f, color, p.name.c_str());

        float below = box.y + box.h + 2.f;
        if (grp.distance) {
            char buf[32];
            snprintf(buf, sizeof(buf), "%.0fm", meters);
            DrawCenteredText(draw, head.x, below, IM_COL32(220, 220, 220, 255), buf);
            below += 14.f;
        }
    }

    ReportState(stats);
}

void DumpBones() {
    DBG_I("bones", "=============== bone dump ===============");

    if (!g::client) {
        DBG_E("bones", "no client.dll base");
        return;
    }

    const uintptr_t entityList = mem::Read<uintptr_t>(g::client + offsets::dwEntityList);
    const uintptr_t localPawn = mem::Read<uintptr_t>(g::client + offsets::dwLocalPlayerPawn);

    if (mem::Valid(localPawn)) {
        const Vector3 origin = sdk::GetAbsOrigin(localPawn);
        bones::Calibrate(localPawn, origin);
        DBG_I("bones", "-- local pawn, origin (%.1f, %.1f, %.1f) --", origin.x, origin.y, origin.z);
        bones::DumpAll(localPawn);
    }

    // One enemy too: their model may number bones differently than the local one.
    for (int i = 1; i <= kMaxPlayers; ++i) {
        sdk::PlayerInfo p;
        if (!sdk::GetPlayer(entityList, i, p) || p.pawn == localPawn)
            continue;
        DBG_I("bones", "-- slot %d '%s', origin (%.1f, %.1f, %.1f) --",
              i, p.name.c_str(), p.origin.x, p.origin.y, p.origin.z);
        bones::DumpAll(p.pawn);
        break;
    }

    DBG_I("bones", "table in use: pelvis=%d chest=%d neck=%d head=%d "
                   "shoulders=%d/%d hips=%d/%d ankles=%d/%d",
          bones::Pelvis, bones::Chest, bones::Neck, bones::Head,
          bones::ArmUpperL, bones::ArmUpperR,
          bones::LegUpperL, bones::LegUpperR,
          bones::AnkleL, bones::AnkleR);
    DBG_I("bones", "=========================================");
}

void Dump() {
    DBG_I("dump", "=============== entity dump ===============");
    gameinfo::ReportBuildMatch();

    if (!g::client) {
        DBG_E("dump", "client.dll base is 0 - nothing to inspect");
        return;
    }

    const uintptr_t entityList = mem::Read<uintptr_t>(g::client + offsets::dwEntityList);
    const uintptr_t chunk0     = mem::Read<uintptr_t>(entityList + 0x10);
    const uintptr_t localPawn  = mem::Read<uintptr_t>(g::client + offsets::dwLocalPlayerPawn);
    const uintptr_t localCtrl  = mem::Read<uintptr_t>(g::client + offsets::dwLocalPlayerController);

    DBG_I("dump", "client     0x%llX", U(g::client));
    DBG_I("dump", "entityList 0x%llX   chunk[0] 0x%llX", U(entityList), U(chunk0));
    DBG_I("dump", "localPawn  0x%llX   localCtrl 0x%llX", U(localPawn), U(localCtrl));

    entlist::Calibrate(entityList, localCtrl, localPawn);
    DBG_I("dump", "list layout: chunkArray=0x%llX stride=0x%llX instance=0x%llX calibrated=%d",
          U(entlist::layout.chunkArray), U(entlist::layout.stride),
          U(entlist::layout.instance), entlist::layout.calibrated ? 1 : 0);

    // The local pawn is the one entity guaranteed to be a real pawn while in a
    // match, so its fields are the cleanest test of the schema offsets.
    if (mem::Valid(localPawn)) {
        const int hp   = mem::Read<int>(localPawn + schema::m_iHealth);
        const int team = mem::Read<uint8_t>(localPawn + schema::m_iTeamNum);
        const int life = mem::Read<uint8_t>(localPawn + schema::m_lifeState);
        const uintptr_t node = mem::Read<uintptr_t>(localPawn + schema::m_pGameSceneNode);
        const Vector3 o = sdk::GetAbsOrigin(localPawn);

        DBG_I("dump", "local pawn: hp=%d team=%d lifeState=%d node=0x%llX origin=(%.1f, %.1f, %.1f)",
              hp, team, life, U(node), o.x, o.y, o.z);

        const bool sane = hp > 0 && hp <= 100 && (team == team::T || team == team::CT) &&
                          mem::Valid(node) && !o.IsZero();
        dbg::Print(sane ? dbg::Level::Ok : dbg::Level::Error, "dump",
                   "local pawn fields look %s -> schema offsets are %s",
                   sane ? "sane" : "WRONG", sane ? "correct" : "stale or you are not spawned");
    } else {
        DBG_W("dump", "no local pawn - in the main menu, or dwLocalPlayerPawn is stale");
    }

    int players = 0, all = 0, shown = 0;
    for (int i = 1; i <= 2048; ++i) {
        const uintptr_t e = sdk::GetEntityByIndex(entityList, i);
        if (!mem::Valid(e))
            continue;

        ++all;
        if (i > kMaxPlayers)
            continue;
        ++players;
        if (shown++ >= 12)
            continue;

        const uint8_t  alive = mem::Read<uint8_t>(e + schema::m_bPawnIsAlive);
        const uint32_t handle = mem::Read<uint32_t>(e + schema::m_hPlayerPawn);
        const uintptr_t pawn = sdk::GetEntityByHandle(entityList, handle);
        const int hp   = mem::Valid(pawn) ? mem::Read<int>(pawn + schema::m_iHealth) : -1;
        const int team = mem::Valid(pawn) ? mem::Read<uint8_t>(pawn + schema::m_iTeamNum) : -1;
        const uint64_t steam = mem::Read<uint64_t>(e + schema::m_steamID);
        const std::string name =
            mem::ReadString(mem::Read<uintptr_t>(e + schema::m_sSanitizedPlayerName), 32);

        DBG_I("dump", "slot %2d ent=0x%llX alive=%u hPawn=0x%08X pawn=0x%llX hp=%d team=%d "
                      "steam=%llu name='%s'",
              i, U(e), alive, handle, U(pawn), hp, team,
              static_cast<unsigned long long>(steam), name.c_str());
    }

    DBG_I("dump", "non-null entities: %d in slots 1..%d, %d in 1..2048",
          players, kMaxPlayers, all);

    if (all == 0)
        DBG_E("dump", "traversal returns nothing - dwEntityList or the chunk layout is wrong");
    else if (players == 0)
        DBG_W("dump", "entities exist but player slots are empty - not in a match?");

    DBG_I("dump", "faulting reads so far: %lu", mem::faults);

    // Also drop the resolved offsets and the raw schema to disk so they can be
    // sent back without copying the console by hand.
    resolve::DumpToFile();

    DBG_I("dump", "==========================================");
}

} // namespace esp
