#include <algorithm>

#include "features/rage/damage.h"

namespace rage {
namespace damage {

int Required(const cfg::RageWeaponConfig& weaponCfg, bool overrideHeld, int enemyHealth) {
    const int configured = overrideHeld ? weaponCfg.minDamageOverride : weaponCfg.minDamage;
    if (configured <= 0)
        return 0;

    if (configured <= 100)
        return configured;

    // Above 100 the value stops being an absolute number and becomes
    // "kill, plus this much margin". 110 against a 70 HP enemy asks for 80.
    const int margin = configured - 100;
    const int health = enemyHealth > 0 ? enemyHealth : 100;
    return health + margin;
}

autowall::Shot Estimate(const weapon::Info& info, float distance, bool head,
                        const weapon::Armor& armor, int enemyHealth) {
    const cfg::Ragebot& r = cfg::c.rage;
    return autowall::Estimate(info, distance, head, armor, enemyHealth,
                              r.autowall, std::max(0, r.autowallWalls));
}

} // namespace damage
} // namespace rage
