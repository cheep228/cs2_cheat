#include <cstring>

#include "features/rage/weapon_settings.h"

namespace rage {
namespace weapons {

const char* const kGroupNames[cfg::GroupCount] = {
    "Global", "Pistols", "Heavy pistols", "SMG",
    "Rifles", "Shotguns", "Snipers", "Auto snipers",
};

namespace {

// Two groups cannot be derived from weapon::Kind alone: the heavy pistols are
// pistols with rifle-grade damage, and the auto snipers are snipers that fire
// like rifles. Both are small closed sets, so they are named outright.
bool IsHeavyPistol(const char* name) {
    return std::strcmp(name, "deagle") == 0 || std::strcmp(name, "revolver") == 0;
}

bool IsAutoSniper(const char* name) {
    return std::strcmp(name, "g3sg1") == 0 || std::strcmp(name, "scar20") == 0;
}

} // namespace

int GroupFor(int defIndex) {
    const weapon::Info& info = weapon::ById(defIndex);
    if (info.damage <= 0.f)
        return cfg::GroupGlobal;

    switch (info.kind) {
    case weapon::Kind::Pistol:
        return IsHeavyPistol(info.name) ? cfg::GroupHeavyPistols : cfg::GroupPistols;

    case weapon::Kind::Smg:
        return cfg::GroupSmg;

    case weapon::Kind::Rifle:
        return cfg::GroupRifles;

    // Machineguns behave like rifles for every setting the ragebot exposes,
    // and a group of two guns nobody rages with is not worth the menu space.
    case weapon::Kind::Machinegun:
        return cfg::GroupRifles;

    case weapon::Kind::Shotgun:
        return cfg::GroupShotguns;

    case weapon::Kind::Sniper:
        return IsAutoSniper(info.name) ? cfg::GroupAutoSnipers : cfg::GroupSnipers;

    default:
        return cfg::GroupGlobal;
    }
}

const char* GroupName(int group) {
    if (group < 0 || group >= cfg::GroupCount)
        return "-";
    return kGroupNames[group];
}

const cfg::RageWeaponConfig& Active(int defIndex, int* resolvedGroup) {
    const cfg::Ragebot& r = cfg::c.rage;

    int group = cfg::GroupGlobal;
    if (r.perWeaponGroups) {
        const int candidate = GroupFor(defIndex);
        // A disabled group is a request to fall back to Global, not a request
        // to stop aiming - the per-group Enabled box only chooses who owns the
        // settings. Whether the ragebot runs at all is cfg::Ragebot::enabled.
        if (candidate != cfg::GroupGlobal && r.groups[candidate].enabled)
            group = candidate;
    }

    if (resolvedGroup)
        *resolvedGroup = group;
    return r.groups[group];
}

} // namespace weapons
} // namespace rage
