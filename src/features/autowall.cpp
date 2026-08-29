#include <algorithm>
#include <cmath>

#include "features/autowall.h"

namespace {

// CS2 scales damage by the range modifier once per 500 units travelled.
float RangeFalloff(const weapon::Info& info, float distance) {
    if (distance > info.maxRange)
        return 0.f;
    return std::pow(info.rangeModifier, distance / 500.f);
}

// The engine's own armour scaling, kept in the same shape as the game's
// ScaleDamage so the numbers line up with what players see.
float ApplyArmor(float damage, float armorRatio, const weapon::Armor& armor,
                 bool head) {
    if (armor.value <= 0)
        return damage;
    if (head && !armor.helmet)
        return damage;   // kevlar alone does not protect the head

    constexpr float kArmorBonus = 0.5f;
    constexpr float kArmorScale = 1.f;

    float scaled = damage * armorRatio;
    const float absorbed = (damage - scaled) * kArmorScale * kArmorBonus;
    if (absorbed > static_cast<float>(armor.value))
        scaled = damage - static_cast<float>(armor.value) / kArmorBonus;

    return scaled < 0.f ? 0.f : scaled;
}

// What a shot keeps after N walls. Modelled on the engine's per-surface loss:
// each wall costs a share of the remaining damage, scaled by how well the
// weapon penetrates. Not a trace - see the header.
float WallLoss(const weapon::Info& info, int walls) {
    if (walls <= 0)
        return 1.f;

    const float pen = info.penetration <= 0.f ? 1.f : info.penetration;
    float keep = 1.f;
    for (int i = 0; i < walls; ++i)
        keep *= 0.42f + 0.14f * pen;   // pen 1 -> 0.56, pen 3 -> 0.84

    return keep;
}

} // namespace

namespace autowall {

Shot Estimate(const weapon::Info& info, float distance, bool head,
              const weapon::Armor& armor, int targetHealth,
              bool assumeWalls, int walls) {
    Shot out;
    if (info.damage <= 0.f)
        return out;

    float base = info.damage * RangeFalloff(info, distance);
    if (head)
        base *= info.headshotMul;

    out.freeAir = ApplyArmor(base, info.armorRatio, armor, head);

    const int used = assumeWalls ? std::max(0, walls) : 0;
    out.wallsUsed = used;
    out.damage = out.freeAir * WallLoss(info, used);
    out.lethal = out.damage >= static_cast<float>(targetHealth);
    return out;
}

} // namespace autowall
