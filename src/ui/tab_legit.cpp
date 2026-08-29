#include "core/globals.h"
#include "features/aimbot.h"
#include "ui/skeet.h"
#include "ui/tabs.h"

namespace ui {
namespace {

const char* const kSubs[] = { "AIMBOT", "TARGETS" };

void DrawAimbot() {
    cfg::Aimbot& a = cfg::c.aim;
    const float w = skeet::ColumnWidth(2);

    skeet::BeginGroupBox("Aimbot", ImVec2(w, 0.f));
    skeet::Checkbox("Enabled", &a.enabled);
    skeet::Keybind("Aim key", &a.key);
    skeet::SliderFloat("Field of view", &a.fov, 0.5f, 30.f, "%.1f deg");
    skeet::SliderFloat("Smoothing", &a.smooth, 1.f, 25.f, "%.1f");
    skeet::TextDim("1 snaps instantly; higher drags slower.");
    skeet::EndGroupBox();

    ImGui::SameLine();

    skeet::BeginGroupBox("Hitboxes", ImVec2(w, 0.f));
    skeet::MultiCombo("Aim at", a.parts, aimbot::kPartNames, cfg::PartCount);

    int selected = 0;
    for (bool on : a.parts)
        selected += on ? 1 : 0;

    if (selected == 0)
        skeet::TextColored(skeet::col::kBad, "Nothing selected - aimbot idle.");
    else
        skeet::TextDim("%d of %d parts active; nearest wins.", selected, cfg::PartCount);
    skeet::EndGroupBox();

    skeet::BeginGroupBox("Visuals", ImVec2(w, 0.f));
    skeet::Checkbox("Draw FOV circle", &a.drawFov);
    skeet::ColorPicker("FOV colour", &a.fovColor);
    skeet::Checkbox("Draw target line", &a.drawTarget);
    skeet::ColorPicker("Target colour", &a.targetColor);
    skeet::EndGroupBox();

    ImGui::SameLine();

    skeet::BeginGroupBox("Status", ImVec2(w, 0.f));
    const aimbot::State& s = aimbot::state;
    skeet::Text("Key held:   %s", s.active ? "yes" : "no");
    skeet::Text("Candidates: %d", s.candidates);
    if (s.hasTarget)
        skeet::Text("Target:     slot %d, %s, %.1f deg", s.targetSlot,
                    aimbot::kPartNames[s.targetPart], s.targetFov);
    else
        skeet::TextDim("no target in FOV");
    skeet::EndGroupBox();
}

void DrawTargets() {
    cfg::Aimbot& a = cfg::c.aim;
    const float w = skeet::ColumnWidth(2);

    skeet::BeginGroupBox("Filters", ImVec2(w, 0.f));
    skeet::Checkbox("Team check", &a.teamCheck);
    skeet::Checkbox("Players", &a.onPlayers);
    skeet::Checkbox("Bots", &a.onBots);
    skeet::Checkbox("Alive only", &a.requireAlive);
    skeet::EndGroupBox();

    ImGui::SameLine();

    skeet::BeginGroupBox("Notes", ImVec2(w, 0.f));
    skeet::TextDim("The legit aimbot moves your real view angles");
    skeet::TextDim("with smoothing, which is what makes it legit.");
    skeet::TextDim("For snap aiming with damage and hitchance");
    skeet::TextDim("gates, use the RAGE tab instead.");
    skeet::EndGroupBox();
}

} // namespace

SubTabs LegitSubs() {
    return { kSubs, IM_ARRAYSIZE(kSubs) };
}

void LegitDraw(int sub) {
    if (sub == 0)
        DrawAimbot();
    else
        DrawTargets();
}

} // namespace ui
