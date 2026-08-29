#include "core/globals.h"
#include "features/chams.h"
#include "features/esp.h"
#include "sdk/bones.h"
#include "ui/theme.h"
#include "ui/tabs.h"

namespace ui {
namespace {

const char* const kSubs[] = { "PLAYERS", "BOTS", "WORLD" };

const char* const kBoxTypes[]   = { "2D", "Corners", "Filled" };
const char* const kChamsModes[] = { "Glow", "Metallic", "Plastic", "Latex" };

// One ESP group, laid out as three groupboxes. Players and bots are the same
// struct, so they get the same code instead of two near-identical copies.
void DrawGroup(const char* id, cfg::EspGroup& grp) {
    ImGui::PushID(id);
    const float w = theme::ColumnWidth(2);

    theme::BeginGroupBox("Box", ImVec2(w, 0.f));
    theme::Checkbox("Enabled", &grp.enabled);
    theme::Checkbox("Bounding box", &grp.box);
    theme::Combo("Box type", &grp.boxType, kBoxTypes, IM_ARRAYSIZE(kBoxTypes));
    theme::ColorPicker("Box colour", &grp.color);
    theme::EndGroupBox();

    ImGui::SameLine();

    theme::BeginGroupBox("Information", ImVec2(w, 0.f));
    theme::Checkbox("Name", &grp.name);
    theme::Checkbox("Health bar", &grp.health);
    theme::Checkbox("Distance", &grp.distance);
    theme::Checkbox("Snapline", &grp.snapline);
    theme::EndGroupBox();

    theme::BeginGroupBox("Skeleton", ImVec2(w, 0.f));
    theme::Checkbox("Skeleton", &grp.skeleton);
    theme::ColorPicker("Skeleton colour", &grp.skeletonColor);
    theme::Checkbox("Aim history", &grp.aimHistory);
    theme::ColorPicker("History colour", &grp.aimHistoryColor);
    theme::TextDim("Ghost skeleton trailing by the delay");
    theme::TextDim("set under WORLD.");
    theme::EndGroupBox();

    ImGui::SameLine();

    theme::BeginGroupBox("Highlight", ImVec2(w, 0.f));
    theme::Checkbox("Glow", &grp.glow);
    theme::ColorPicker("Glow colour", &grp.glowColor);
    theme::Checkbox("Chams", &grp.chams);
    theme::Combo("Chams mode", &grp.chamsMode, kChamsModes, IM_ARRAYSIZE(kChamsModes));
    theme::ColorPicker("Chams colour", &grp.chamsColor);
    theme::TextDim("Chams overrides glow while both are on.");
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

    ImGui::SameLine();

    theme::BeginGroupBox("Bones", ImVec2(w, 0.f));
    if (bones::layout.calibrated)
        theme::TextColored(theme::col::kGood, "calibrated at modelState+0x%X",
                           static_cast<unsigned>(bones::layout.boneArray));
    else
        theme::TextColored(theme::col::kBad, "not calibrated - skeleton, aimbot");
    if (!bones::layout.calibrated)
        theme::TextColored(theme::col::kBad, "and ragebot are all idle");
    if (theme::Button("Dump bones to console (F10)"))
        esp::DumpBones();
    theme::EndGroupBox();

    theme::BeginGroupBox("Glow tuning", ImVec2(w, 0.f));
    cfg::GlowTuning& t = cfg::c.glowTune;
    theme::TextDim("Which fields CS2 needs for a highlight");
    theme::TextDim("varies by build; tune it here.");
    theme::Checkbox("Use chams-mode presets", &t.usePresets);
    theme::SliderInt("Glow type", &t.type, 0, 6);
    theme::SliderInt("Range", &t.range, 0, 20000);
    theme::SliderInt("Range min", &t.rangeMin, 0, 2000);
    theme::SliderInt("Glow team", &t.team, -1, 3);
    theme::EndGroupBox();

    ImGui::SameLine();

    theme::BeginGroupBox("Glow fields", ImVec2(w, 0.f));
    theme::Checkbox("Flashing", &t.flashing);
    theme::Checkbox("Eligible for highlight", &t.eligible);
    theme::Checkbox("Write float colour", &t.floatColor);
    theme::Checkbox("Refresh glow time", &t.glowTime);
    theme::Checkbox("Verify writes", &t.verify);
    theme::Text("pawns written last frame: %d", chams::lastApplied);
    theme::EndGroupBox();
}

} // namespace

SubTabs VisualsSubs() {
    return { kSubs, IM_ARRAYSIZE(kSubs) };
}

void VisualsDraw(int sub) {
    switch (sub) {
    case 0:  DrawGroup("players", cfg::c.players); break;
    case 1:  DrawGroup("bots", cfg::c.bots); break;
    default: DrawWorld(); break;
    }
}

} // namespace ui
