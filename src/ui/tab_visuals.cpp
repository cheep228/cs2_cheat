#include "core/globals.h"
#include "features/chams.h"
#include "features/esp.h"
#include "features/world.h"
#include "sdk/bones.h"
#include "ui/theme.h"
#include "ui/tabs.h"

namespace ui {
namespace {

const char* const kSubs[] = { "PLAYERS", "WORLD" };

const char* const kBoxTypes[]   = { "2D", "Corners", "Filled" };
const char* const kChamsModes[] = { "Glow", "Metallic", "Plastic", "Latex" };

// One ESP group for every player pawn. Bots are players: there is no second
// struct and no second set of widgets, so a setting changed here applies to
// humans and fake clients alike. Team filtering is separate and still lives
// under WORLD.
void DrawPlayers() {
    cfg::EspGroup& grp = cfg::c.players;
    ImGui::PushID("players");
    const float w = theme::ColumnWidth(2);

    theme::BeginGroupBox("Box", ImVec2(w, 0.f));
    theme::Checkbox("Enabled", &grp.enabled);
    theme::Checkbox("Bounding box", &grp.box);
    theme::Combo("Box type", &grp.boxType, kBoxTypes, IM_ARRAYSIZE(kBoxTypes));
    theme::ColorPicker("Box colour", &grp.color);
    theme::TextDim("Applies to humans and bots alike.");
    theme::EndGroupBox();

    ImGui::SameLine(0.f, 16.f * ImGui::GetIO().GUIScale);

    theme::BeginGroupBox("Information", ImVec2(w, 0.f));
    theme::Checkbox("Name", &grp.name);
    theme::Checkbox("Health bar", &grp.health);
    theme::Checkbox("Weapon", &grp.weapon);
    theme::Checkbox("Distance", &grp.distance);
    theme::Checkbox("Snapline", &grp.snapline);
    theme::Checkbox("Flags", &grp.flags);
    theme::ColorPicker("Flags colour", &grp.flagsColor);
    theme::TextDim("Flags: AIR, SCOPED, FLASH, RELOAD, K/HK.");
    theme::EndGroupBox();

    theme::BeginGroupBox("Skeleton", ImVec2(w, 0.f));
    theme::Checkbox("Skeleton", &grp.skeleton);
    theme::ColorPicker("Skeleton colour", &grp.skeletonColor);
    theme::Checkbox("Aim history", &grp.aimHistory);
    theme::ColorPicker("History colour", &grp.aimHistoryColor);
    theme::TextDim("Ghost skeleton trailing by the delay");
    theme::TextDim("set under WORLD.");
    theme::EndGroupBox();

    ImGui::SameLine(0.f, 16.f * ImGui::GetIO().GUIScale);

    theme::BeginGroupBox("Highlight", ImVec2(w, 0.f));
    theme::Checkbox("Glow", &grp.glow);
    theme::ColorPicker("Glow colour", &grp.glowColor);
    theme::Checkbox("Chams", &grp.chams);
    theme::Combo("Chams mode", &grp.chamsMode, kChamsModes, IM_ARRAYSIZE(kChamsModes));
    theme::ColorPicker("Chams colour", &grp.chamsColor);
    theme::TextColored(theme::col::kBad, "Backend is the game's glow property,");
    theme::TextColored(theme::col::kBad, "not a material override - no separate");
    theme::TextColored(theme::col::kBad, "visible/occluded pass exists yet.");
    theme::EndGroupBox();

    ImGui::PopID();
}

void DrawWorld() {
    const float w = theme::ColumnWidth(2);

    theme::BeginGroupBox("ESP", ImVec2(w, 0.f));
    theme::Checkbox("ESP enabled", &cfg::c.espEnabled);
    theme::Checkbox("Team check", &cfg::c.teamCheck);
    theme::SliderFloat("Max distance", &cfg::c.maxDistance, 10.f, 400.f, "%.0f m");
    theme::SliderFloat("Aim history delay", &cfg::c.aimHistoryDelayMs, 0.f, 500.f,
                       "%.0f ms");
    theme::SliderFloat("Skeleton thickness", &cfg::c.skeletonThickness, 0.5f, 4.f,
                       "%.1f");
    theme::EndGroupBox();

    ImGui::SameLine(0.f, 16.f * ImGui::GetIO().GUIScale);

    theme::BeginGroupBox("World visuals", ImVec2(w, 0.f));
    cfg::World& wcfg = cfg::c.world;

    theme::Checkbox("Anti smoke", &wcfg.antiSmoke);
    if (wcfg.antiSmoke)
        theme::Text("tracked %d, suppressed %d", world::stats.smokesTracked,
                    world::stats.smokesSuppressed);
    else
        theme::TextDim("Rewrites the smoke's render start tick.");

    theme::Separator();

    theme::Checkbox("Anti flash", &wcfg.antiFlash);
    theme::SliderInt("Flash amount", &wcfg.flashAmount, 0, 100, "%d%%");
    theme::TextDim("0%% removes it, 100%% is the stock flash.");
    if (wcfg.antiFlash)
        theme::Text("overlay alpha now: %.0f", world::stats.flashOverlay);
    if (!world::stats.localPawnOk)
        theme::TextColored(theme::col::kBad, "no local pawn - idle");

    theme::Separator();

    theme::Checkbox("Bomb ESP", &wcfg.bombEsp);
    theme::Checkbox("Bomb timer", &wcfg.bombTimer);
    theme::ColorPicker("Bomb colour", &wcfg.bombColor);

    if (wcfg.bombEsp || wcfg.bombTimer) {
        const world::Bomb& b = world::bomb;
        theme::Separator();

        // Enough state to tell WHERE it broke: entity not found, found but not
        // projected, or projected but no clock.
        theme::Text("dropped C4: %s", b.droppedFound ? "yes" : "no");
        theme::Text("planted C4: %s", b.found ? "yes" : "no");

        if (b.found) {
            theme::Text("pos %.0f %.0f %.0f", b.origin.x, b.origin.y, b.origin.z);
            theme::Text("site %s   ticking %s   W2S %s",
                        b.site == 0 ? "A" : b.site == 1 ? "B" : "?",
                        b.ticking ? "yes" : "no",
                        b.w2sOk ? "ok" : "FAIL");
            if (b.defused)
                theme::TextColored(theme::col::kGood, "defused");
            else if (b.timerValid)
                theme::Text("blow %.1f  now %.1f  left %.1fs",
                            b.lastBlowTime, b.lastCurTime, b.remaining);
            else
                theme::TextColored(theme::col::kBad, "timer invalid - see console");

            if (b.beingDefused) {
                if (b.defuseValid)
                    theme::TextColored(b.defuseInTime ? theme::col::kGood : theme::col::kBad,
                                       "defuse %.1fs (%s)", b.defuseRemaining,
                                       b.defuseInTime ? "in time" : "too late");
                else
                    theme::TextDim("being defused, countdown unavailable");
            }
        }
    }
    theme::EndGroupBox();

    theme::BeginGroupBox("Bones", ImVec2(w, 0.f));
    if (bones::layout.calibrated)
        theme::TextColored(theme::col::kGood, "calibrated at modelState+0x%X",
                           static_cast<unsigned>(bones::layout.boneArray));
    else
        theme::TextColored(theme::col::kBad, "not calibrated - skeleton and");
    if (!bones::layout.calibrated)
        theme::TextColored(theme::col::kBad, "aimbot are both idle");
    if (theme::Button("Dump bones to console (F10)"))
        esp::DumpBones();
    theme::EndGroupBox();

    ImGui::SameLine(0.f, 16.f * ImGui::GetIO().GUIScale);

    theme::BeginGroupBox("Glow tuning", ImVec2(w, 0.f));
    cfg::GlowTuning& t = cfg::c.glowTune;
    theme::TextDim("Which fields CS2 needs for a highlight");
    theme::TextDim("varies by build; tune it here.");
    theme::Checkbox("Use chams-mode presets", &t.usePresets);
    theme::SliderInt("Glow type", &t.type, 0, 6);
    theme::SliderInt("Range", &t.range, 0, 20000);
    theme::SliderInt("Glow team", &t.team, -1, 3);
    theme::Checkbox("Verify writes", &t.verify);
    theme::Text("pawns written last frame: %d", chams::lastApplied);
    theme::EndGroupBox();
}

} // namespace

SubTabs VisualsSubs() {
    return { kSubs, IM_ARRAYSIZE(kSubs) };
}

void VisualsDraw(int sub) {
    if (sub == 0)
        DrawPlayers();
    else
        DrawWorld();
}

} // namespace ui
