#include "core/globals.h"
#include "features/chams.h"
#include "features/esp.h"
#include "sdk/bones.h"
#include "ui/skeet.h"
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
    const float w = skeet::ColumnWidth(2);

    skeet::BeginGroupBox("Box", ImVec2(w, 0.f));
    skeet::Checkbox("Enabled", &grp.enabled);
    skeet::Checkbox("Bounding box", &grp.box);
    skeet::Combo("Box type", &grp.boxType, kBoxTypes, IM_ARRAYSIZE(kBoxTypes));
    skeet::ColorPicker("Box colour", &grp.color);
    skeet::EndGroupBox();

    ImGui::SameLine();

    skeet::BeginGroupBox("Information", ImVec2(w, 0.f));
    skeet::Checkbox("Name", &grp.name);
    skeet::Checkbox("Health bar", &grp.health);
    skeet::Checkbox("Distance", &grp.distance);
    skeet::Checkbox("Snapline", &grp.snapline);
    skeet::EndGroupBox();

    skeet::BeginGroupBox("Skeleton", ImVec2(w, 0.f));
    skeet::Checkbox("Skeleton", &grp.skeleton);
    skeet::ColorPicker("Skeleton colour", &grp.skeletonColor);
    skeet::Checkbox("Aim history", &grp.aimHistory);
    skeet::ColorPicker("History colour", &grp.aimHistoryColor);
    skeet::TextDim("Ghost skeleton trailing by the delay");
    skeet::TextDim("set under WORLD.");
    skeet::EndGroupBox();

    ImGui::SameLine();

    skeet::BeginGroupBox("Highlight", ImVec2(w, 0.f));
    skeet::Checkbox("Glow", &grp.glow);
    skeet::ColorPicker("Glow colour", &grp.glowColor);
    skeet::Checkbox("Chams", &grp.chams);
    skeet::Combo("Chams mode", &grp.chamsMode, kChamsModes, IM_ARRAYSIZE(kChamsModes));
    skeet::ColorPicker("Chams colour", &grp.chamsColor);
    skeet::TextDim("Chams overrides glow while both are on.");
    skeet::EndGroupBox();

    ImGui::PopID();
}

void DrawWorld() {
    const float w = skeet::ColumnWidth(2);

    skeet::BeginGroupBox("ESP", ImVec2(w, 0.f));
    skeet::Checkbox("ESP enabled", &cfg::c.espEnabled);
    skeet::Checkbox("Team check", &cfg::c.teamCheck);
    skeet::SliderFloat("Max distance", &cfg::c.maxDistance, 10.f, 400.f, "%.0f m");
    skeet::SliderFloat("Aim history delay", &cfg::c.aimHistoryDelayMs, 0.f, 500.f,
                       "%.0f ms");
    skeet::SliderFloat("Skeleton thickness", &cfg::c.skeletonThickness, 0.5f, 4.f,
                       "%.1f");
    skeet::EndGroupBox();

    ImGui::SameLine();

    skeet::BeginGroupBox("Bones", ImVec2(w, 0.f));
    if (bones::layout.calibrated)
        skeet::TextColored(skeet::col::kGood, "calibrated at modelState+0x%X",
                           static_cast<unsigned>(bones::layout.boneArray));
    else
        skeet::TextColored(skeet::col::kBad, "not calibrated - skeleton, aimbot");
    if (!bones::layout.calibrated)
        skeet::TextColored(skeet::col::kBad, "and ragebot are all idle");
    if (skeet::Button("Dump bones to console (F10)"))
        esp::DumpBones();
    skeet::EndGroupBox();

    skeet::BeginGroupBox("Glow tuning", ImVec2(w, 0.f));
    cfg::GlowTuning& t = cfg::c.glowTune;
    skeet::TextDim("Which fields CS2 needs for a highlight");
    skeet::TextDim("varies by build; tune it here.");
    skeet::Checkbox("Use chams-mode presets", &t.usePresets);
    skeet::SliderInt("Glow type", &t.type, 0, 6);
    skeet::SliderInt("Range", &t.range, 0, 20000);
    skeet::SliderInt("Range min", &t.rangeMin, 0, 2000);
    skeet::SliderInt("Glow team", &t.team, -1, 3);
    skeet::EndGroupBox();

    ImGui::SameLine();

    skeet::BeginGroupBox("Glow fields", ImVec2(w, 0.f));
    skeet::Checkbox("Flashing", &t.flashing);
    skeet::Checkbox("Eligible for highlight", &t.eligible);
    skeet::Checkbox("Write float colour", &t.floatColor);
    skeet::Checkbox("Refresh glow time", &t.glowTime);
    skeet::Checkbox("Verify writes", &t.verify);
    skeet::Text("pawns written last frame: %d", chams::lastApplied);
    skeet::EndGroupBox();
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
