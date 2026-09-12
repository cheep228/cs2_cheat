#include "fields.h"

#include "log.h"
#include "schema_system.h"

namespace fields {
namespace {

struct FieldSig {
    const char* cls;
    const char* field;
    ptrdiff_t*  target;
};

const FieldSig kFields[] = {
    { "C_BasePlayerPawn",        "m_pWeaponServices",      &m_pWeaponServices },
    { "CPlayer_WeaponServices",  "m_hActiveWeapon",        &m_hActiveWeapon },
    { "CCSPlayer_WeaponServices", "m_hMyWeapons",          &m_hMyWeapons },
    { "C_EconEntity",            "m_AttributeManager",     &m_AttributeManager },
    { "C_AttributeContainer",    "m_Item",                 &m_Item },
    { "C_EconItemView",          "m_iItemDefinitionIndex", &m_iItemDefinitionIndex },
    { "C_EconItemView",          "m_iItemIDHigh",          &m_iItemIDHigh },
    { "C_EconItemView",          "m_iEntityQuality",       &m_iEntityQuality },
    { "C_EconItemView",          "m_nFallbackPaintKit",    &m_nFallbackPaintKit },
    { "C_EconItemView",          "m_nFallbackSeed",        &m_nFallbackSeed },
    { "C_EconItemView",          "m_flFallbackWear",       &m_flFallbackWear },
    { "C_EconItemView",          "m_nFallbackStatTrak",    &m_nFallbackStatTrak },
    { "C_EconItemView",          "m_szCustomName",         &m_szCustomName },
};

} // namespace

ResolveStats ResolveAll() {
    ResolveStats stats;
    stats.total = static_cast<int>(sizeof(kFields) / sizeof(kFields[0]));

    const uintptr_t scope = schemasys::TypeScope("libclient.so");
    if (!scope) {
        DBG_E("fields", "no libclient.so type scope - cannot resolve econ fields");
        return stats;
    }

    const int classCount = schemasys::ClassCount(scope);
    DBG_I("fields", "%d classes in the libclient.so scope", classCount);
    if (classCount < 100) {
        DBG_E("fields", "suspiciously few classes - the schema layout may not match this build");
        return stats;
    }

    for (const FieldSig& f : kFields) {
        const ptrdiff_t off = schemasys::FieldOffset(scope, f.cls, f.field);
        if (off < 0) {
            DBG_W("fields", "%s::%s not found (yet)", f.cls, f.field);
            continue;
        }
        ++stats.found;
        if (*f.target != off) {
            DBG_OK("fields", "%s::%s = 0x%llX", f.cls, f.field,
                   static_cast<unsigned long long>(off));
        }
        *f.target = off;
    }

    DBG_I("fields", "resolved %d/%d econ fields", stats.found, stats.total);
    return stats;
}

} // namespace fields
