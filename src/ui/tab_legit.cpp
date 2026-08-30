#include "core/globals.h"
#include "core/safe_settings.h"
#include "sdk/visibility.h"
#include "core/game_hooks.h"
#include "features/recoil.h"
#include "features/triggerbot.h"
#include "features/aimbot.h"
#include "sdk/trace.h"
#include "ui/theme.h"
#include "ui/tabs.h"

namespace ui {
namespace {

const char* const kSubs[] = { "AIMBOT", "TARGETS", "TRIGGER", "RECOIL" };

void DrawAimbot() {
    cfg::Aimbot& a = cfg::c.aim;
    const float w = theme::ColumnWidth(2);
    const bool safe = safeset::Active();

    theme::BeginGroupBox("Aimbot", ImVec2(w, 0.f));
    theme::Checkbox("Enabled", &a.enabled);
    theme::Keybind("Aim key", &a.key);

    // While the guard rail is on the slider itself is bounded, so the value
    // cannot be pushed past the limit and then snapped back a frame later.
    const float fovMax = safe ? cfg::SafeSettings::kMaxFov : 30.f;
    const float smoothMin = safe ? cfg::SafeSettings::kMinSmooth : 1.f;
    theme::SliderFloat("Field of view", &a.fov, 0.5f, fovMax, "%.1f deg");
    theme::SliderFloat("Smoothing", &a.smooth, smoothMin, 25.f, "%.1f");
    theme::TextDim("1 snaps instantly; higher drags slower.");
    if (safe) {
        theme::TextColored(theme::col::kAccent, "Safe settings: fov <= %.1f, smooth >= %.1f",
                           cfg::SafeSettings::kMaxFov, cfg::SafeSettings::kMinSmooth);
    }
    theme::EndGroupBox();

    ImGui::SameLine(0.f, 16.f * ImGui::GetIO().GUIScale);

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

    theme::BeginGroupBox("Visibility", ImVec2(w, 0.f));
    if (safe) {
        // Forced on and shown as forced, rather than silently rewritten.
        bool locked = true;
        ImGui::BeginDisabled(true);
        theme::Checkbox("Visibility check", &locked);
        ImGui::EndDisabled();
        theme::TextColored(theme::col::kAccent, "Locked on by Safe settings.");
    } else {
        theme::Checkbox("Visibility check", &a.visibilityCheck);
    }

    if (!a.visibilityCheck) {
        theme::TextDim("Off: targets are taken through walls.");
    } else if (visibility::Available()) {
        theme::TextColored(theme::col::kGood, "Running - spotted mask, slot %d.",
                           visibility::LocalSlot());
        theme::TextDim("Uses the visibility the server already");
        theme::TextDim("computes for you, not a ray trace.");
        theme::TextDim("So it means in your view AND not");
        theme::TextDim("occluded, and answers per player, not");
        theme::TextDim("per aim point.");
    } else {
        theme::TextColored(theme::col::kBad, "NOT RUNNING: spotted state did not");
        theme::TextColored(theme::col::kBad, "resolve on this build.");
        theme::TextDim("See the console for which class failed.");
    }
    theme::EndGroupBox();

    ImGui::SameLine(0.f, 16.f * ImGui::GetIO().GUIScale);

    theme::BeginGroupBox("Status", ImVec2(w, 0.f));
    const aimbot::State& s = aimbot::state;
    theme::Text("Key held:   %s", s.active ? "yes" : "no");
    theme::Text("Candidates: %d", s.candidates);
    if (s.hasTarget)
        theme::Text("Target:     slot %d, %s, %.1f deg", s.targetSlot,
                    aimbot::kPartNames[s.targetPart], s.targetFov);
    else
        theme::TextDim("no target in FOV");
    theme::Separator();
    theme::Text("Rejected (occluded): %d", s.rejectedOccluded);
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

    ImGui::SameLine(0.f, 16.f * ImGui::GetIO().GUIScale);

    theme::BeginGroupBox("Silent aim", ImVec2(w, 0.f));
    theme::Checkbox("Silent aim", &a.silentAim);

    if (!gamehooks::state.createMoveOk) {
        theme::TextColored(theme::col::kBad, "CreateMove hook not installed -");
        theme::TextColored(theme::col::kBad, "the shot angle cannot be reached.");
    } else if (a.silentAim) {
        theme::TextColored(theme::col::kGood, "Running - camera is left alone.");
        theme::TextDim("The angle goes into the command's input");
        theme::TextDim("history, so the shot resolves against the");
        theme::TextDim("target while your view stays put.");
        if (!gamehooks::state.shootAngleOk)
            theme::TextColored(theme::col::kBad, "no history entries this tick");
    } else {
        theme::TextDim("Off: the aimbot moves the camera.");
    }
    theme::EndGroupBox();
}


void DrawTrigger() {
    cfg::TriggerBot& tb = cfg::c.trigger;
    const float w = theme::ColumnWidth(2);

    theme::BeginGroupBox("Trigger bot", ImVec2(w, 0.f));
    theme::Checkbox("Trigger bot", &tb.enabled);
    theme::Keybind("Trigger key", &tb.key);
    theme::TextDim(tb.key == 0 ? "no key: always armed" : "hold to arm");
    theme::SliderInt("Trigger delay", &tb.delayMs, 0, 250, "%d ms");
    theme::TextDim("The same target must stay under the ray");
    theme::TextDim("this long. Measured in game time, so no");
    theme::TextDim("thread ever sleeps and fps does not matter.");
    theme::EndGroupBox();

    ImGui::SameLine(0.f, 16.f * ImGui::GetIO().GUIScale);

    theme::BeginGroupBox("Filters", ImVec2(w, 0.f));
    theme::Checkbox("Team check", &tb.teamCheck);
    theme::Checkbox("Visibility check", &tb.visibilityCheck);
    if (tb.visibilityCheck && !visibility::Available())
        theme::TextColored(theme::col::kBad, "spotted state unavailable");
    theme::Separator();
    theme::TextDim("Hitgroups");
    for (int i = 0; i < triggerbot::GroupCount; ++i)
        theme::Checkbox(triggerbot::GroupName(i), &tb.groups[i]);
    theme::EndGroupBox();

    theme::BeginGroupBox("How it picks", ImVec2(w, 0.f));
    theme::TextDim("A ray is cast from your eye along your");
    theme::TextDim("real view angles and intersected against");
    theme::TextDim("capsules built from the enemy's own bones -");
    theme::TextDim("the same ones the skeleton ESP draws.");
    theme::TextDim("Nearest hit along the ray wins, and it");
    theme::TextDim("reports which hitgroup was crossed.");
    theme::Separator();
    theme::TextColored(theme::col::kBad, "Limit: no world trace exists here,");
    theme::TextColored(theme::col::kBad, "so a wall between you and the target");
    theme::TextColored(theme::col::kBad, "does not stop the ray.");
    theme::TextDim("Visibility check falls back to the spotted");
    theme::TextDim("mask, which answers per player, not per");
    theme::TextDim("point. See sdk/visibility.h.");
    theme::EndGroupBox();

    ImGui::SameLine(0.f, 16.f * ImGui::GetIO().GUIScale);

    theme::BeginGroupBox("Status", ImVec2(w, 0.f));
    const triggerbot::Stats& s = triggerbot::stats;

    if (!gamehooks::state.createMoveOk) {
        theme::TextColored(theme::col::kBad, "CreateMove hook not installed -");
        theme::TextColored(theme::col::kBad, "the attack cannot be written.");
    } else {
        theme::Text("key held:     %s", s.keyHeld ? "yes" : "no");
        theme::Text("weapon:       %s", s.weapon);
        theme::Text("clip %d   next tick %d   now %d",
                    s.clip, s.nextAttackTick, s.tickCount);
        theme::Text("weapon ready: %s", s.weaponReady ? "yes" : "no");
        theme::Separator();
        theme::Text("target found: %s", s.targetFound ? "yes" : "no");
        if (s.targetFound) {
            theme::Text("entity:       %s (slot %d)", s.targetName, s.targetSlot);
            theme::Text("hitgroup:     %s", triggerbot::GroupName(s.hitGroup));
            theme::Text("distance:     %.0f units", s.hitDistance);
            theme::Text("team valid:   %s", s.teamValid ? "yes" : "no");
            theme::Text("visibility:   %s", s.visible ? "yes" : "no");
            theme::Text("held for:     %.0f ms", s.heldForMs);
            theme::Text("delay done:   %s", s.delayElapsed ? "yes" : "no");
        }
        theme::Separator();
        const bool fired = s.reject == triggerbot::Reject::Fired;
        theme::TextColored(fired ? theme::col::kGood : theme::col::kBad,
                           "%s", triggerbot::RejectName(s.reject));
        theme::Text("shots fired:  %llu", static_cast<unsigned long long>(s.shotsFired));
    }
    theme::EndGroupBox();
}


void DrawRecoil() {
    cfg::RecoilControl& rc = cfg::c.recoil;
    const float w = theme::ColumnWidth(2);

    theme::BeginGroupBox("Recoil control", ImVec2(w, 0.f));
    theme::Checkbox("Recoil control", &rc.enabled);
    theme::SliderInt("Vertical", &rc.vertical, 0, 100, "%d%%");
    theme::SliderInt("Horizontal", &rc.horizontal, 0, 100, "%d%%");
    theme::SliderInt("Start after shots", &rc.startAfterShots, 1, 5);
    theme::SliderInt("Smooth", &rc.smooth, 0, 10);
    theme::TextDim("Smooth spreads the correction over the");
    theme::TextDim("following commands, not over frames.");
    theme::EndGroupBox();

    ImGui::SameLine(0.f, 16.f * ImGui::GetIO().GUIScale);

    theme::BeginGroupBox("What it does", ImVec2(w, 0.f));
    theme::TextDim("Pulls the view back against the punch the");
    theme::TextDim("game reports for your own pawn. Not a");
    theme::TextDim("recorded spray pattern and not random.");
    theme::Separator();
    theme::TextColored(theme::col::kBad, "100%% is not a motionless weapon.");
    theme::TextDim("Recoil is the systematic walk of the gun;");
    theme::TextDim("spread is the per-bullet scatter inside the");
    theme::TextDim("cone. This cancels the first and leaves");
    theme::TextDim("the second alone.");
    theme::Separator();
    theme::TextDim("Works with the aimbot off - it compensates");
    theme::TextDim("your own fire, so FOV and visibility do not");
    theme::TextDim("apply to it.");
    theme::EndGroupBox();

    theme::BeginGroupBox("Punch scale", ImVec2(w, 0.f));
    theme::SliderFloat("Scale", &rc.scale, 0.5f, 2.5f, "%.2f");
    theme::TextDim("How m_aimPunchAngle maps onto the view.");
    theme::TextDim("Left at 1.00 because CS2 reports the punch");
    theme::TextDim("in view degrees already - CS:GO needed 2.");
    theme::TextDim("Exposed so it can be checked against the");
    theme::TextDim("game rather than assumed. If a long spray");
    theme::TextDim("at 100/100 still climbs, raise it; if the");
    theme::TextDim("view is dragged below the aim, lower it.");
    theme::EndGroupBox();

    ImGui::SameLine(0.f, 16.f * ImGui::GetIO().GUIScale);

    theme::BeginGroupBox("Status", ImVec2(w, 0.f));
    const recoil::Stats& s = recoil::stats;

    if (!gamehooks::state.createMoveOk) {
        theme::TextColored(theme::col::kBad, "CreateMove hook not installed -");
        theme::TextColored(theme::col::kBad, "the view cannot be corrected per tick.");
    } else {
        theme::Text("weapon:      %s", s.weapon);
        theme::Text("shots fired: %d   tick %d", s.shotsFired, s.tickCount);
        theme::Separator();
        theme::Text("current  P %.3f  Y %.3f", s.currentPunch.x, s.currentPunch.y);
        theme::Text("previous P %.3f  Y %.3f", s.previousPunch.x, s.previousPunch.y);
        theme::Text("delta    P %.3f  Y %.3f", s.deltaPunch.x, s.deltaPunch.y);
        theme::Separator();
        theme::Text("strength V %d%%  H %d%%", rc.vertical, rc.horizontal);
        theme::Text("applied  P %.3f  Y %.3f", s.appliedPitch, s.appliedYaw);
        if (rc.smooth > 0)
            theme::Text("pending  P %.3f  Y %.3f", s.pendingPitch, s.pendingYaw);
        theme::Separator();
        theme::TextColored(s.running ? theme::col::kGood : theme::col::kBad,
                           "%s", s.reason);

        // If the punch never moves the field is wrong, and saying so beats
        // pretending the compensation is working.
        if (rc.enabled && s.shotsFired > 1 &&
            s.currentPunch.x == 0.f && s.currentPunch.y == 0.f)
            theme::TextColored(theme::col::kBad, "punch reads 0 while firing -");
        if (rc.enabled && s.shotsFired > 1 &&
            s.currentPunch.x == 0.f && s.currentPunch.y == 0.f)
            theme::TextColored(theme::col::kBad, "m_aimPunchAngle did not resolve");
    }
    theme::EndGroupBox();
}

} // namespace

SubTabs LegitSubs() {
    return { kSubs, IM_ARRAYSIZE(kSubs) };
}

void LegitDraw(int sub) {
    switch (sub) {
    case 0:  DrawAimbot();  break;
    case 1:  DrawTargets(); break;
    case 2:  DrawTrigger(); break;
    default: DrawRecoil();  break;
    }
}

} // namespace ui
