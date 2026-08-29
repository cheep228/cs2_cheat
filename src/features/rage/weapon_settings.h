#pragma once
#include "core/globals.h"
#include "sdk/weapon.h"

// Which per-group config applies to the gun in the local player's hands.
//
// The groups are a view over cfg::c.rage.groups[], never a copy: editing
// "Rifles" in the menu edits the same struct the scanner reads next frame.
namespace rage {
namespace weapons {

// cfg::WeaponGroup for a weapon definition index. Anything without ballistics
// (knife, grenade, zeus) returns GroupGlobal, but the caller should already
// have rejected it - see Ragebot's NoWeapon gate.
int GroupFor(int defIndex);

// "Rifles", "Auto snipers", ... Indexed by cfg::WeaponGroup.
const char* GroupName(int group);
extern const char* const kGroupNames[cfg::GroupCount];

// The config the ragebot should obey for this weapon. Falls back to the Global
// group when per-weapon groups are off or when the matching group is disabled,
// so a disabled group means "use Global", never "no settings at all".
const cfg::RageWeaponConfig& Active(int defIndex, int* resolvedGroup);

} // namespace weapons
} // namespace rage
