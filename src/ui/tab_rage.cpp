#include <Windows.h>

#include "core/globals.h"
#include "features/rage/hitbox_scanner.h"
#include "features/rage/weapon_settings.h"
#include "features/ragebot.h"
#include "ui/skeet.h"
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
    const float w = skeet::ColumnWidth(2);

    skeet::BeginGroupBox("Ragebot", ImVec2(w, 0.f));
    skeet::Checkbox("Enabled", &r.enabled);
    skeet::Keybind("Rage key", &r.key);
    skeet::TextDim(r.key == 0 ? "no key: always on" : "hold to engage");
    skeet::Separator();
    skeet::Combo("Target selection", &r.targetMode, kTargetModes,
                 IM_ARRAYSIZE(kTargetModes));
    skeet::Checkbox("Per-weapon groups", &r.perWeaponGroups);
    skeet::TextDim(r.perWeaponGroups ? "each group keeps its own settings"
                                     : "every weapon uses Global");
    skeet::EndGroupBox();

    ImGui::SameLine();

    skeet::BeginGroupBox("Keys", ImVec2(w, 0.f));
    skeet::Keybind("Min damage override", &r.minDamageOverrideKey);
    skeet::Keybind("Force body", &r.forceBodyKey);
    skeet::Separator();
    skeet::Checkbox("Penetration model", &r.autowall);
    if (r.autowall)
        skeet::SliderInt("Assumed walls", &r.autowallWalls, 1, 3);
    skeet::TextDim("Models damage kept after N walls.");
    skeet::TextDim("There is no ray trace: it never");
    skeet::TextDim("claims a wall is actually there.");
    skeet::EndGroupBox();

    skeet::BeginGroupBox("Debug", ImVec2(w, 0.f));
    skeet::Checkbox("Debug rendering", &r.debug);
    skeet::Checkbox("Draw FOV", &r.drawFov);
    skeet::Checkbox("Draw target", &r.drawTarget);
    skeet::Checkbox("Draw scanner points", &r.drawPoints);
    skeet::Separator();
    skeet::ColorPicker("FOV colour", &r.fovColor);
    skeet::ColorPicker("Target colour", &r.targetColor);
    skeet::ColorPicker("Point colour", &r.pointColor);
    skeet::EndGroupBox();

    ImGui::SameLine();

    // The panel that answers "why is it not shooting?" without guesswork.
    const rage::Diagnostics& d = ragebot::state.diag;

    skeet::BeginGroupBox("Status", ImVec2(w, 0.f));
    skeet::Text("Stage:      %s", rage::StageName(d.stage));
    skeet::Text("Weapon:     %s", d.weapon);
    skeet::Text("Group:      %s%s", d.group, d.groupActive ? "" : " (fallback)");
    skeet::Separator();
    skeet::Text("Enemies:    %d  (in FOV %d)", d.candidates, d.inFov);
    skeet::Text("Points:     %d tested, %d passed", d.pointsTested, d.pointsPassed);

    if (d.hasTarget) {
        skeet::Separator();
        skeet::Text("Target:     %s (slot %d, %d hp)", d.targetName, d.targetSlot,
                    d.targetHealth);
        skeet::Text("Hitbox:     %s", rage::hitboxes::Name(d.hitbox));
        skeet::Text("Damage:     %.0f   required %d", d.damage, d.requiredDamage);
        skeet::Text("Hitchance:  %.0f%%  required %d%%", d.hitchance,
                    d.requiredHitchance);
        skeet::Text("FOV:        %.2f of %.0f", d.fov, d.maxFov);
        if (d.lethal)
            skeet::TextColored(skeet::col::kGood, "Lethal point");
    }

    skeet::Separator();
    const bool good = d.result && d.result[0] == 'V';
    skeet::TextColored(good ? skeet::col::kGood : skeet::col::kBad, "Result: %s",
                       d.result ? d.result : "-");

    if (d.overrideHeld)
        skeet::TextColored(skeet::col::kAccent, "min damage override held");
    if (d.forceBodyHeld)
        skeet::TextColored(skeet::col::kAccent, "force body held");
    if (!d.visibilityKnown && d.hasTarget)
        skeet::TextDim("visibility: unknown (no trace)");
    skeet::EndGroupBox();
}

void DrawWeapons() {
    cfg::Ragebot& r = cfg::c.rage;
    const float w = skeet::ColumnWidth(2);

    skeet::BeginGroupBox("Weapon group", ImVec2(w, 0.f));
    skeet::Combo("Editing", &g_editedGroup, rage::weapons::kGroupNames, cfg::GroupCount);
    if (g_editedGroup == cfg::GroupGlobal)
        skeet::TextDim("Fallback for every weapon.");
    else
        skeet::TextDim("Unchecked = this group falls back to Global.");
    skeet::EndGroupBox();

    ImGui::SameLine();

    cfg::RageWeaponConfig& wc = r.groups[g_editedGroup];

    skeet::BeginGroupBox("Aimbot", ImVec2(w, 0.f));
    if (g_editedGroup != cfg::GroupGlobal)
        skeet::Checkbox("Enabled", &wc.enabled);
    skeet::SliderFloat("Field of view", &wc.fov, 1.f, 180.f, "%.0f deg");
    skeet::SliderInt("Hitchance", &wc.hitchance, 0, 100, "%d %%");
    skeet::SliderInt("Minimum damage", &wc.minDamage, 1, 130);
    skeet::SliderInt("Min damage override", &wc.minDamageOverride, 1, 130);
    skeet::TextDim("Above 100 means enemy HP + the excess.");
    skeet::EndGroupBox();

    skeet::BeginGroupBox("Target", ImVec2(w, 0.f));
    for (int i = 0; i < cfg::HitboxCount; ++i)
        skeet::Checkbox(rage::hitboxes::kNames[i], &wc.hitboxes[i]);
    skeet::EndGroupBox();

    ImGui::SameLine();

    skeet::BeginGroupBox("Multipoint", ImVec2(w, 0.f));
    skeet::Checkbox("Multipoint", &wc.multipoint);
    skeet::SliderFloat("Head point scale", &wc.headScale, 0.f, 1.f, "%.0f%%");
    skeet::SliderFloat("Body point scale", &wc.bodyScale, 0.f, 1.f, "%.0f%%");
    skeet::TextDim("0% aims at the centre, 100% at the edge.");
    skeet::TextDim("Head gets 4 points, torso 3, limbs 1.");
    skeet::EndGroupBox();

    skeet::BeginGroupBox("Body aim", ImVec2(w, 0.f));
    skeet::Checkbox("Prefer body", &wc.preferBody);
    skeet::TextDim("Body scores higher, head still allowed.");
    skeet::Checkbox("Force body", &wc.forceBody);
    skeet::TextDim("Head excluded from the scan entirely.");
    skeet::Checkbox("Body aim if lethal", &wc.bodyAimLethal);
    skeet::Checkbox("Headshot only", &wc.headshotOnly);
    skeet::TextDim("Scan is built from the head alone.");
    skeet::EndGroupBox();

    ImGui::SameLine();

    skeet::BeginGroupBox("Automation", ImVec2(w, 0.f));
    skeet::Checkbox("Auto fire", &wc.autoFire);
    skeet::TextDim(wc.autoFire ? "fires once a point clears every gate"
                               : "aims only; you pull the trigger");
    skeet::Combo("Auto stop", &wc.autoStop, kAutoStop, IM_ARRAYSIZE(kAutoStop));
    skeet::TextDim("Runs only with a point already chosen.");
    skeet::TextDim("Full also releases W/A/S/D (key-ups only).");
    skeet::Checkbox("Auto scope", &wc.autoScope);
    skeet::TextDim("Ignored on weapons without a scope.");
    skeet::EndGroupBox();
}

void DrawAntiAim() {
    const float w = skeet::ColumnWidth(1);

    skeet::BeginGroupBox("Anti-aim", ImVec2(w, 0.f));
    skeet::TextColored(skeet::col::kBad, "Not implemented - and not faked.");
    skeet::Separator();
    skeet::TextDim("Anti-aim means sending view angles to the server that differ");
    skeet::TextDim("from the ones you render locally. That requires writing the");
    skeet::TextDim("usercmd before it is signed and sent, which a DX11 present");
    skeet::TextDim("hook never sees: this module runs once per rendered frame,");
    skeet::TextDim("after input has already gone out.");
    skeet::Separator();
    skeet::TextDim("Writing dwViewAngles - all this overlay can do - moves your");
    skeet::TextDim("real angles, so the server and your screen agree and nothing");
    skeet::TextDim("is desynced. Toggles here would be decoration.");
    skeet::Separator();
    skeet::TextDim("The missing piece is a CUserCmd / CInput hook. Once that");
    skeet::TextDim("exists, yaw/pitch modifiers, fake lag and desync belong");
    skeet::TextDim("here and nowhere else.");
    skeet::EndGroupBox();
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
