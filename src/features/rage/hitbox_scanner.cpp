#include "features/rage/hitbox_scanner.h"
#include "sdk/bones.h"

namespace rage {
namespace hitboxes {

const char* const kNames[cfg::HitboxCount] = {
    "Head", "Neck", "Chest", "Stomach", "Pelvis", "Arms", "Legs", "Feet",
};

namespace {

// Nominal radii in Source units. CS2 keeps the real hitbox set inside the
// model data, which this overlay does not parse; these come from the measured
// bone spacing in the F10 dump and are only used to spread multipoint samples
// and to size the hitchance target, never to claim a hit.
struct Definition {
    int   bones[2];     // -1 terminates a single-bone hitbox
    float radius;
    bool  head;
    bool  limb;
};

const Definition kDefs[cfg::HitboxCount] = {
    /* Head    */ { { bones::Head,      -1 },              3.9f, true,  false },
    /* Neck    */ { { bones::Neck,      -1 },              3.0f, false, false },
    /* Chest   */ { { bones::Chest,     -1 },              9.0f, false, false },
    /* Stomach */ { { bones::Spine2,    -1 },              8.0f, false, false },
    /* Pelvis  */ { { bones::Pelvis,    -1 },              7.0f, false, false },
    /* Arms    */ { { bones::ArmUpperL, bones::ArmUpperR }, 4.0f, false, true  },
    /* Legs    */ { { bones::LegUpperL, bones::LegUpperR }, 5.0f, false, true  },
    /* Feet    */ { { bones::AnkleL,    bones::AnkleR },    3.5f, false, true  },
};

} // namespace

int Build(const cfg::RageWeaponConfig& weaponCfg, bool forceBodyHeld, Entry* out, int max) {
    if (!out || max <= 0)
        return 0;

    // Headshot-only wins outright: the list is the head and nothing else, so
    // no other bone is ever read, scored or drawn.
    if (weaponCfg.headshotOnly) {
        if (!weaponCfg.hitboxes[cfg::HitboxHead])
            return 0;   // asked for head only, then unchecked the head
        out[0].hitbox = cfg::HitboxHead;
        out[0].bone   = kDefs[cfg::HitboxHead].bones[0];
        out[0].radius = kDefs[cfg::HitboxHead].radius;
        out[0].head   = true;
        out[0].limb   = false;
        return 1;
    }

    const bool excludeHead = weaponCfg.forceBody || forceBodyHeld;

    int count = 0;
    for (int box = 0; box < cfg::HitboxCount && count < max; ++box) {
        if (!weaponCfg.hitboxes[box])
            continue;

        const Definition& def = kDefs[box];
        if (def.head && excludeHead)
            continue;

        for (int i = 0; i < 2 && count < max; ++i) {
            if (def.bones[i] < 0)
                break;
            Entry& e = out[count++];
            e.hitbox = box;
            e.bone   = def.bones[i];
            e.radius = def.radius;
            e.head   = def.head;
            e.limb   = def.limb;
        }
    }
    return count;
}

const char* Name(int hitbox) {
    if (hitbox < 0 || hitbox >= cfg::HitboxCount)
        return "-";
    return kNames[hitbox];
}

} // namespace hitboxes
} // namespace rage
