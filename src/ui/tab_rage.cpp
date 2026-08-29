#include <Windows.h>

#include "core/globals.h"
#include "features/rage/hitbox_scanner.h"
#include "features/rage/weapon_settings.h"
#include "features/ragebot.h"
#include "ui/theme.h"
#include "ui/tabs.h"

namespace ui {
namespace {

const char* const kSubs[] = { "GENERAL", "WEAPONS", "ANTI-AIM" };

const char* const kTargetModes[] = {
    "Closest FOV", "Lowest HP", "Closest distance", "Highest damage",
};

const char* const kAutoStop[] = { "Off", "Normal", "Full" };

// Which weapon group the WEAPONS sub-tab is editing. Menu state, not config:
// it must not end up in a saved slot.
int g_editedGroup = cfg::GroupGlobal;

void DrawGeneral() {
    cfg::Ragebot& r = cfg::c.rage;
    const float w = theme::ColumnWidth(2);

    theme::BeginGroupBox("Ragebot", ImVec2(w, 0.f));
    theme::Checkbox("Enabled", &r.enabled);
    theme::Keybind("Rage key", &r.key);
    theme::TextDim(r.key == 0 ? "no key: always on" : "hold to engage");
    theme::Separator();
    theme::Combo("Target selection", &r.targetMode, kTargetModes,
                 IM_ARRAYSIZE(kTargetModes));
    theme::Checkbox("Per-weapon groups", &r.perWeaponGroups);
    theme::TextDim(r.perWeaponGroups ? "each group keeps its own settings"
                                     : "every weapon uses Global");
    theme::EndGroupBox();

    ImGui::SameLine();

    theme::BeginGroupBox("Keys", ImVec2(w, 0.f));
    theme::Keybind("Min damage override", &r.minDamageOverrideKey);
    theme::Keybind("Force body", &r.forceBodyKey);
    theme::Separator();
    theme::Checkbox("Penetration model", &r.autowall);
    if (r.autowall)
        theme::SliderInt("Assumed walls", &r.autowallWalls, 1, 3);
    theme::TextDim("Models damage kept after N walls.");
    theme::TextDim("There is no ray trace: it never");
    theme::TextDim("claims a wall is actually there.");
    theme::EndGroupBox();

    theme::BeginGroupBox("Debug", ImVec2(w, 0.f));
    theme::Checkbox("Debug rendering", &r.debug);
    theme::Checkbox("Draw FOV", &r.drawFov);
    theme::Checkbox("Draw target", &r.drawTarget);
    theme::Checkbox("Draw scanner points", &r.drawPoints);
    theme::Separator();
    theme::ColorPicker("FOV colour", &r.fovColor);
    theme::ColorPicker("Target colour", &r.targetColor);
    theme::ColorPicker("Point colour", &r.pointColor);
    theme::EndGroupBox();

    ImGui::SameLine();

    // The panel that answers "why is it not shooting?" without guesswork.
    const rage::Diagnostics& d = ragebot::state.diag;

    theme::BeginGroupBox("Status", ImVec2(w, 0.f));
    theme::Text("Stage:      %s", rage::StageName(d.stage));
    theme::Text("Weapon:     %s", d.weapon);
    theme::Text("Group:      %s%s", d.group, d.groupActive ? "" : " (fallback)");
    theme::Separator();
    theme::Text("Enemies:    %d  (in FOV %d)", d.candidates, d.inFov);
    theme::Text("Points:     %d tested, %d passed", d.pointsTested, d.pointsPassed);

    if (d.hasTarget) {
        theme::Separator();
        theme::Text("Target:     %s (slot %d, %d hp)", d.targetName, d.targetSlot,
                    d.targetHealth);
        theme::Text("Hitbox:     %s", rage::hitboxes::Name(d.hitbox));
        theme::Text("Damage:     %.0f   required %d", d.damage, d.requiredDamage);
        theme::Text("Hitchance:  %.0f%%  required %d%%", d.hitchance,
                    d.requiredHitchance);
        theme::Text("FOV:        %.2f of %.0f", d.fov, d.maxFov);
        if (d.lethal)
            theme::TextColored(theme::col::kGood, "Lethal point");
    }

    theme::Separator();
    const bool good = d.result && d.result[0] == 'V';
    theme::TextColored(good ? theme::col::kGood : theme::col::kBad, "Result: %s",
                       d.result ? d.result : "-");

    if (d.overrideHeld)
        theme::TextColored(theme::col::kAccent, "min damage override held");
    if (d.forceBodyHeld)
        theme::TextColored(theme::col::kAccent, "force body held");
    if (!d.visibilityKnown && d.hasTarget)
        theme::TextDim("visibility: unknown (no trace)");
    theme::EndGroupBox();
}

void DrawWeapons() {
    cfg::Ragebot& r = cfg::c.rage;
    const float w = theme::ColumnWidth(2);

    theme::BeginGroupBox("Weapon group", ImVec2(w, 0.f));
    theme::Combo("Editing", &g_editedGroup, rage::weapons::kGroupNames, cfg::GroupCount);
    if (g_editedGroup == cfg::GroupGlobal)
        theme::TextDim("Fallback for every weapon.");
    else
        theme::TextDim("Unchecked = this group falls back to Global.");
    theme::EndGroupBox();

    ImGui::SameLine();

    cfg::RageWeaponConfig& wc = r.groups[g_editedGroup];

    theme::BeginGroupBox("Aimbot", ImVec2(w, 0.f));
    if (g_editedGroup != cfg::GroupGlobal)
        theme::Checkbox("Enabled", &wc.enabled);
    theme::SliderFloat("Field of view", &wc.fov, 1.f, 180.f, "%.0f deg");
    theme::SliderInt("Hitchance", &wc.hitchance, 0, 100, "%d %%");
    theme::SliderInt("Minimum damage", &wc.minDamage, 1, 130);
    theme::SliderInt("Min damage override", &wc.minDamageOverride, 1, 130);
    theme::TextDim("Above 100 means enemy HP + the excess.");
    theme::EndGroupBox();

    theme::BeginGroupBox("Target", ImVec2(w, 0.f));
    for (int i = 0; i < cfg::HitboxCount; ++i)
        theme::Checkbox(rage::hitboxes::kNames[i], &wc.hitboxes[i]);
    theme::EndGroupBox();

    ImGui::SameLine();

    theme::BeginGroupBox("Multipoint", ImVec2(w, 0.f));
    theme::Checkbox("Multipoint", &wc.multipoint);
    theme::SliderFloat("Head point scale", &wc.headScale, 0.f, 1.f, "%.0f%%");
    theme::SliderFloat("Body point scale", &wc.bodyScale, 0.f, 1.f, "%.0f%%");
    theme::TextDim("0% aims at the centre, 100% at the edge.");
    theme::TextDim("Head gets 4 points, torso 3, limbs 1.");
    theme::EndGroupBox();

    theme::BeginGroupBox("Body aim", ImVec2(w, 0.f));
    theme::Checkbox("Prefer body", &wc.preferBody);
    theme::TextDim("Body scores higher, head still allowed.");
    theme::Checkbox("Force body", &wc.forceBody);
    theme::TextDim("Head excluded from the scan entirely.");
    theme::Checkbox("Body aim if lethal", &wc.bodyAimLethal);
    theme::Checkbox("Headshot only", &wc.headshotOnly);
    theme::TextDim("Scan is built from the head alone.");
    theme::EndGroupBox();

    ImGui::SameLine();

    theme::BeginGroupBox("Automation", ImVec2(w, 0.f));
    theme::Checkbox("Auto fire", &wc.autoFire);
    theme::TextDim(wc.autoFire ? "fires once a point clears every gate"
                               : "aims only; you pull the trigger");
    theme::Combo("Auto stop", &wc.autoStop, kAutoStop, IM_ARRAYSIZE(kAutoStop));
    theme::TextDim("Runs only with a point already chosen.");
    theme::TextDim("Full also releases W/A/S/D (key-ups only).");
    theme::Checkbox("Auto scope", &wc.autoScope);
    theme::TextDim("Ignored on weapons without a scope.");
    theme::EndGroupBox();
}

void DrawAntiAim() {
    const float w = theme::ColumnWidth(1);

    theme::BeginGroupBox("Anti-aim", ImVec2(w, 0.f));
    theme::TextColored(theme::col::kBad, "Not implemented - and not faked.");
    theme::Separator();
    theme::TextDim("Anti-aim means sending view angles to the server that differ");
    theme::TextDim("from the ones you render locally. That requires writing the");
    theme::TextDim("usercmd before it is signed and sent, which a DX11 present");
    theme::TextDim("hook never sees: this module runs once per rendered frame,");
    theme::TextDim("after input has already gone out.");
    theme::Separator();
    theme::TextDim("Writing dwViewAngles - all this overlay can do - moves your");
    theme::TextDim("real angles, so the server and your screen agree and nothing");
    theme::TextDim("is desynced. Toggles here would be decoration.");
    theme::Separator();
    theme::TextDim("The missing piece is a CUserCmd / CInput hook. Once that");
    theme::TextDim("exists, yaw/pitch modifiers, fake lag and desync belong");
    theme::TextDim("here and nowhere else.");
    theme::EndGroupBox();
}

} // namespace

SubTabs RageSubs() {
    return { kSubs, IM_ARRAYSIZE(kSubs) };
}

void RageDraw(int sub) {
    switch (sub) {
    case 0:  DrawGeneral(); break;
    case 1:  DrawWeapons(); break;
    default: DrawAntiAim(); break;
    }
}

} // namespace ui
