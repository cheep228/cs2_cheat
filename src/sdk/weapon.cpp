#include <Windows.h>

#include "core/globals.h"
#include "sdk/entity.h"
#include "sdk/entitylist.h"
#include "sdk/offsets.h"
#include "sdk/weapon.h"

namespace {

using weapon::Kind;

// Item definition indices are the one thing in CS2 that does not move between
// builds, so the table is keyed on them rather than on a name string.
const weapon::Info kTable[] = {
    // name          kind             dmg  armor  pen  rangeMod  range  hs    spread  scope
    { "deagle",      Kind::Pistol,     63, 0.930f, 2.f, 0.810f, 8192.f, 4.f, 0.0016f, false },
    { "elite",       Kind::Pistol,     38, 0.500f, 1.f, 0.810f, 8192.f, 4.f, 0.0022f, false },
    { "fiveseven",   Kind::Pistol,     32, 0.965f, 1.f, 0.885f, 8192.f, 4.f, 0.0018f, false },
    { "glock",       Kind::Pistol,     30, 0.500f, 1.f, 0.990f, 8192.f, 4.f, 0.0019f, false },
    { "ak47",        Kind::Rifle,      36, 0.775f, 2.f, 0.980f, 8192.f, 4.f, 0.0009f, false },
    { "aug",         Kind::Rifle,      28, 0.900f, 2.f, 0.990f, 8192.f, 4.f, 0.0008f, true  },
    { "awp",         Kind::Sniper,    115, 0.975f, 3.f, 0.990f, 8192.f, 4.f, 0.0002f, true  },
    { "famas",       Kind::Rifle,      30, 0.700f, 2.f, 0.970f, 8192.f, 4.f, 0.0009f, false },
    { "g3sg1",       Kind::Sniper,     80, 0.825f, 3.f, 0.980f, 8192.f, 4.f, 0.0004f, true  },
    { "galilar",     Kind::Rifle,      30, 0.775f, 2.f, 0.980f, 8192.f, 4.f, 0.0010f, false },
    { "m249",        Kind::Machinegun, 32, 0.800f, 2.f, 0.970f, 8192.f, 4.f, 0.0014f, false },
    { "m4a4",        Kind::Rifle,      33, 0.700f, 2.f, 0.970f, 8192.f, 4.f, 0.0008f, false },
    { "mac10",       Kind::Smg,        29, 0.500f, 1.f, 0.820f, 8192.f, 4.f, 0.0022f, false },
    { "p90",         Kind::Smg,        26, 0.690f, 1.f, 0.840f, 8192.f, 4.f, 0.0019f, false },
    { "mp5sd",       Kind::Smg,        27, 0.500f, 1.f, 0.850f, 8192.f, 4.f, 0.0018f, false },
    { "ump45",       Kind::Smg,        35, 0.650f, 1.f, 0.810f, 8192.f, 4.f, 0.0018f, false },
    { "xm1014",      Kind::Shotgun,    20, 0.800f, 1.f, 0.700f, 3000.f, 4.f, 0.0500f, false },
    { "bizon",       Kind::Smg,        27, 0.500f, 1.f, 0.840f, 8192.f, 4.f, 0.0019f, false },
    { "mag7",        Kind::Shotgun,    30, 0.750f, 1.f, 0.700f, 3000.f, 4.f, 0.0500f, false },
    { "negev",       Kind::Machinegun, 35, 0.750f, 2.f, 0.980f, 8192.f, 4.f, 0.0016f, false },
    { "sawedoff",    Kind::Shotgun,    32, 0.750f, 1.f, 0.700f, 3000.f, 4.f, 0.0500f, false },
    { "tec9",        Kind::Pistol,     33, 0.900f, 1.f, 0.810f, 8192.f, 4.f, 0.0020f, false },
    { "hkp2000",     Kind::Pistol,     35, 0.500f, 1.f, 0.790f, 8192.f, 4.f, 0.0018f, false },
    { "mp7",         Kind::Smg,        29, 0.500f, 1.f, 0.870f, 8192.f, 4.f, 0.0018f, false },
    { "mp9",         Kind::Smg,        26, 0.500f, 1.f, 0.840f, 8192.f, 4.f, 0.0017f, false },
    { "nova",        Kind::Shotgun,    26, 0.500f, 1.f, 0.700f, 3000.f, 4.f, 0.0500f, false },
    { "p250",        Kind::Pistol,     38, 0.645f, 1.f, 0.750f, 8192.f, 4.f, 0.0018f, false },
    { "scar20",      Kind::Sniper,     80, 0.825f, 3.f, 0.990f, 8192.f, 4.f, 0.0004f, true  },
    { "sg553",       Kind::Rifle,      30, 0.900f, 2.f, 0.985f, 8192.f, 4.f, 0.0009f, true  },
    { "ssg08",       Kind::Sniper,     88, 0.850f, 3.f, 0.980f, 8192.f, 4.f, 0.0003f, true  },
    { "m4a1_s",      Kind::Rifle,      38, 0.700f, 2.f, 0.990f, 8192.f, 4.f, 0.0008f, false },
    { "usp_s",       Kind::Pistol,     35, 0.500f, 1.f, 0.790f, 8192.f, 4.f, 0.0017f, false },
    { "cz75a",       Kind::Pistol,     33, 0.965f, 1.f, 0.810f, 8192.f, 4.f, 0.0019f, false },
    { "revolver",    Kind::Pistol,     86, 0.885f, 2.f, 0.870f, 8192.f, 4.f, 0.0015f, false },
};

const weapon::Info kUnknown{};

// Definition index -> row in kTable. Anything not listed (knives, grenades,
// the zeus) has no ballistics the ragebot can reason about.
int RowFor(int defIndex) {
    switch (defIndex) {
    case 1:  return 0;   case 2:  return 1;   case 3:  return 2;
    case 4:  return 3;   case 7:  return 4;   case 8:  return 5;
    case 9:  return 6;   case 10: return 7;   case 11: return 8;
    case 13: return 9;   case 14: return 10;  case 16: return 11;
    case 17: return 12;  case 19: return 13;  case 23: return 14;
    case 24: return 15;  case 25: return 16;  case 26: return 17;
    case 27: return 18;  case 28: return 19;  case 29: return 20;
    case 30: return 21;  case 32: return 22;  case 33: return 23;
    case 34: return 24;  case 35: return 25;  case 36: return 26;
    case 38: return 27;  case 39: return 28;  case 40: return 29;
    case 60: return 30;  case 61: return 31;  case 63: return 32;
    case 64: return 33;
    default: return -1;
    }
}

} // namespace

namespace weapon {

const Info& ById(int defIndex) {
    const int row = RowFor(defIndex);
    if (row < 0)
        return kUnknown;
    return kTable[row];
}

Active Local(uintptr_t localPawn) {
    Active a;
    if (!mem::Valid(localPawn))
        return a;

    const uintptr_t services = mem::Read<uintptr_t>(localPawn + schema::m_pWeaponServices);
    if (!mem::Valid(services))
        return a;

    const uintptr_t entityList = mem::Read<uintptr_t>(g::client + offsets::dwEntityList);
    const uint32_t handle = mem::Read<uint32_t>(services + schema::m_hActiveWeapon);
    const uintptr_t wep = sdk::GetEntityByHandle(entityList, handle);
    if (!mem::Valid(wep))
        return a;

    const uintptr_t item = wep + schema::m_AttributeManager + schema::m_Item;
    const int defIndex = mem::Read<uint16_t>(item + schema::m_iItemDefinitionIndex);
    if (defIndex <= 0)
        return a;

    a.ok = true;
    a.entity = wep;
    a.defIndex = defIndex;
    a.info = &ById(defIndex);
    a.accuracyPenalty = mem::Read<float>(wep + schema::m_fAccuracyPenalty);
    a.reloading = mem::Read<bool>(wep + schema::m_bInReload);
    a.clip = mem::Read<int32_t>(wep + schema::m_iClip1);
    a.nextAttackTick = mem::Read<int32_t>(wep + schema::m_nNextPrimaryAttackTick);
    a.scoped = mem::Read<bool>(localPawn + schema::m_bIsScoped);

    // A wild penalty means the offset moved; treat it as "no live spread data"
    // instead of poisoning the hitchance estimate.
    if (!(a.accuracyPenalty >= 0.f) || a.accuracyPenalty > 1.f)
        a.accuracyPenalty = 0.f;

    return a;
}

Armor ArmorOf(uintptr_t pawn) {
    Armor out;
    out.value = mem::Read<int>(pawn + schema::m_ArmorValue);
    if (out.value < 0 || out.value > 100)
        out.value = 0;

    const uintptr_t items = mem::Read<uintptr_t>(pawn + schema::m_pItemServices);
    if (mem::Valid(items))
        out.helmet = mem::Read<bool>(items + schema::m_bHasHelmet);
    return out;
}

} // namespace weapon
