#include "core/globals.h"
#include "features/aimbot.h"
#include "ui/theme.h"
#include "ui/tabs.h"

namespace ui {
namespace {

const char* const kSubs[] = { "AIMBOT", "TARGETS" };

void DrawAimbot() {
    cfg::Aimbot& a = cfg::c.aim;
    const float w = theme::ColumnWidth(2);

    theme::BeginGroupBox("Aimbot", ImVec2(w, 0.f));
    theme::Checkbox("Enabled", &a.enabled);
    theme::Keybind("Aim key", &a.key);
    theme::SliderFloat("Field of view", &a.fov, 0.5f, 30.f, "%.1f deg");
    theme::SliderFloat("Smoothing", &a.smooth, 1.f, 25.f, "%.1f");
    theme::TextDim("1 snaps instantly; higher drags slower.");
    theme::EndGroupBox();

    ImGui::SameLine();

    theme::BeginGroupBox("Hitboxes", ImVec2(w, 0.f));
    theme::MultiCombo("Aim at", a.parts, aimbot::kPartNames, cfg::PartCount);

    int selected = 0;
    for (bool on : a.parts)
        selected += on ? 1 : 0;

    if (selected == 0)
        theme::TextColored(theme::col::kBad, "Nothing selected - aimbot idle.");
    else
        theme::TextDim("%d of %d parts active; nearest wins.", selected, cfg::PartCount);
    theme::EndGroupBox();

    theme::BeginGroupBox("Visuals", ImVec2(w, 0.f));
    theme::Checkbox("Draw FOV circle", &a.drawFov);
    theme::ColorPicker("FOV colour", &a.fovColor);
    theme::Checkbox("Draw target line", &a.drawTarget);
    theme::ColorPicker("Target colour", &a.targetColor);
    theme::EndGroupBox();

    ImGui::SameLine();

    theme::BeginGroupBox("Status", ImVec2(w, 0.f));
    const aimbot::State& s = aimbot::state;
    theme::Text("Key held:   %s", s.active ? "yes" : "no");
    theme::Text("Candidates: %d", s.candidates);
    if (s.hasTarget)
        theme::Text("Target:     slot %d, %s, %.1f deg", s.targetSlot,
                    aimbot::kPartNames[s.targetPart], s.targetFov);
    else
        theme::TextDim("no target in FOV");
    theme::EndGroupBox();
}

void DrawTargets() {
    cfg::Aimbot& a = cfg::c.aim;
    const float w = theme::ColumnWidth(2);

    theme::BeginGroupBox("Filters", ImVec2(w, 0.f));
    theme::Checkbox("Team check", &a.teamCheck);
    theme::Checkbox("Players", &a.onPlayers);
    theme::Checkbox("Bots", &a.onBots);
    theme::Checkbox("Alive only", &a.requireAlive);
    theme::EndGroupBox();

    ImGui::SameLine();

    theme::BeginGroupBox("Notes", ImVec2(w, 0.f));
    theme::TextDim("The legit aimbot moves your real view angles");
    theme::TextDim("with smoothing, which is what makes it legit.");
    theme::TextDim("For snap aiming with damage and hitchance");
    theme::TextDim("gates, use the RAGE tab instead.");
    theme::EndGroupBox();
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
