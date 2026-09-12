#include "skinchanger.h"

#include "config.h"
#include "entity.h"
#include "fields.h"
#include "log.h"
#include "mem.h"
#include "modules.h"
#include "offsets.h"

namespace skinchanger {
namespace {

uintptr_t g_clientBase = 0;
bool      g_fieldsResolved = false;

bool EnsureReady() {
    if (!g_clientBase) {
        const modules::Region client = modules::Find("libclient.so");
        if (!client.base)
            return false;
        g_clientBase = client.base;
        DBG_OK("skinchanger", "libclient.so base @ 0x%llX",
               static_cast<unsigned long long>(g_clientBase));
    }

    if (!g_fieldsResolved) {
        fields::ResolveAll();
        g_fieldsResolved = fields::Ready();
        if (!g_fieldsResolved)
            return false;
    }

    static bool warnedOffsets = false;
    if (!offsets::Ready()) {
        if (!warnedOffsets) {
            warnedOffsets = true;
            DBG_E("skinchanger", "offsets::dwEntityList / dwLocalPlayerPawn are not filled "
                                  "in - see linux/README.md. Staying idle.");
        }
        return false;
    }

    return true;
}

// Writes the configured fallback skin data into one weapon's econ item.
// Idempotent - safe (and cheap) to call again every tick even when nothing
// changed, which sidesteps needing to detect weapon pickup/drop separately.
void ApplyToWeapon(uintptr_t weaponEntity) {
    if (!mem::Valid(weaponEntity))
        return;

    const uintptr_t item = weaponEntity + fields::m_AttributeManager + fields::m_Item;
    const int defIndex = mem::Read<uint16_t>(item + fields::m_iItemDefinitionIndex);
    if (defIndex <= 0)
        return;

    const config::SkinEntry* entry = config::Find(defIndex);
    if (!entry)
        return;

    // Marks the item as "not a real Steam economy item" - the client then
    // renders from the m_nFallback* fields below instead of looking the item
    // up in the (real) inventory cache. See skinchanger.h for what this does
    // and does not do.
    mem::Write<int32_t>(item + fields::m_iItemIDHigh, -1);
    mem::Write<int32_t>(item + fields::m_nFallbackPaintKit, entry->paintKit);
    mem::Write<int32_t>(item + fields::m_nFallbackSeed, entry->seed);
    mem::Write<float>(item + fields::m_flFallbackWear, entry->wear);
    if (fields::m_nFallbackStatTrak >= 0)
        mem::Write<int32_t>(item + fields::m_nFallbackStatTrak, entry->statTrak);
    if (fields::m_szCustomName >= 0 && !entry->nameTag.empty())
        mem::WriteString(item + fields::m_szCustomName, entry->nameTag, 32);
}

void ApplyToLoadout(uintptr_t pawn, uintptr_t entityList) {
    const uintptr_t services = mem::Read<uintptr_t>(pawn + fields::m_pWeaponServices);
    if (!mem::Valid(services))
        return;

    const uint32_t activeHandle = mem::Read<uint32_t>(services + fields::m_hActiveWeapon);
    ApplyToWeapon(entlist::GetByHandle(entityList, activeHandle));

    if (fields::m_hMyWeapons < 0)
        return;

    // CUtlVector<CHandle<C_BasePlayerWeapon>>: count at +0x0, data at +0x8 -
    // same convention the schema system walk itself relies on.
    const uintptr_t vec = services + fields::m_hMyWeapons;
    const int count = mem::Read<int>(vec);
    const uintptr_t data = mem::Read<uintptr_t>(vec + 0x8);
    if (count <= 0 || count > 64 || !mem::Valid(data))
        return;

    for (int i = 0; i < count; ++i) {
        const uint32_t handle = mem::Read<uint32_t>(data + 4ull * i);
        ApplyToWeapon(entlist::GetByHandle(entityList, handle));
    }
}

} // namespace

void Tick() {
    if (!EnsureReady())
        return;

    config::PollReload(config::DefaultPath());

    const uintptr_t entityList = mem::Read<uintptr_t>(g_clientBase + offsets::dwEntityList);
    const uintptr_t localPawn  = mem::Read<uintptr_t>(g_clientBase + offsets::dwLocalPlayerPawn);
    const uintptr_t localCtrl  = offsets::dwLocalPlayerController
                                      ? mem::Read<uintptr_t>(g_clientBase + offsets::dwLocalPlayerController)
                                      : 0;

    if (!mem::Valid(entityList) || !mem::Valid(localPawn))
        return;

    if (!entlist::layout.calibrated && !entlist::Calibrate(entityList, localCtrl, localPawn))
        return;

    ApplyToLoadout(localPawn, entityList);
}

} // namespace skinchanger
