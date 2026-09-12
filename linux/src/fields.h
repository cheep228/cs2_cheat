#pragma once
#include <cstddef>

// Schema field offsets needed to find a weapon's econ item and spoof how it
// renders. Every value here starts at -1 ("unresolved") and is filled in by
// ResolveAll() from the game's own live schema system - never baked as a
// number, because unlike offsets.h's three globals, these genuinely do not
// need to be: the schema system can look any of them up by class+field name
// on its own, on any build, on any platform.
//
// Field names match the Windows project's src/sdk/offsets.h (schema::...)
// for the ones both projects need, plus the CEconItemView "fallback" fields
// a skin changer needs that project does not use yet. These extra names are
// the same ones cs2-dumper's own schema dumps report for C_EconItemView -
// they are Valve's public, network-serialized item-preview fields, not
// something invented for this project.
namespace fields {

// C_BasePlayerPawn
inline ptrdiff_t m_pWeaponServices = -1;

// CPlayer_WeaponServices
inline ptrdiff_t m_hActiveWeapon = -1;
// CCSPlayer_WeaponServices - full loadout, not just the one held right now.
inline ptrdiff_t m_hMyWeapons = -1;

// C_EconEntity -> C_AttributeContainer -> C_EconItemView
inline ptrdiff_t m_AttributeManager     = -1;
inline ptrdiff_t m_Item                 = -1;
inline ptrdiff_t m_iItemDefinitionIndex = -1;

// C_EconItemView. m_iItemIDHigh == -1 marks the item as not a real Steam
// economy item, which is what makes the client fall back to rendering the
// m_nFallback* fields below instead of looking up a real inventory entry -
// this is the same mechanism CS2's own "preview in game" / workshop test
// tooling uses, not a bug being abused.
inline ptrdiff_t m_iItemIDHigh      = -1;
inline ptrdiff_t m_iEntityQuality   = -1;
inline ptrdiff_t m_nFallbackPaintKit  = -1;
inline ptrdiff_t m_nFallbackSeed      = -1;
inline ptrdiff_t m_flFallbackWear     = -1;
inline ptrdiff_t m_nFallbackStatTrak  = -1;
inline ptrdiff_t m_szCustomName       = -1;

struct ResolveStats {
    int total = 0;
    int found = 0;
};

// Walks libclient.so's schema type scope and fills in every field above.
// Safe to call repeatedly - already-resolved fields are left alone, so a
// transient miss on one call gets picked up on the next.
ResolveStats ResolveAll();

inline bool Ready() {
    return m_AttributeManager >= 0 && m_Item >= 0 && m_iItemDefinitionIndex >= 0 &&
           m_iItemIDHigh >= 0 && m_nFallbackPaintKit >= 0 && m_nFallbackSeed >= 0 &&
           m_flFallbackWear >= 0 && m_pWeaponServices >= 0 && m_hActiveWeapon >= 0;
}

} // namespace fields
