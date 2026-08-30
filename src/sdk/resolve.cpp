#include <Windows.h>
#include <cstdio>

#include "core/log.h"
#include "sdk/entity.h"
#include "sdk/offsets.h"
#include "sdk/resolve.h"
#include "sdk/schema.h"
#include "sdk/sigscan.h"

namespace {

resolve::Result g_last;

// ---------------------------------------------------------------------------
// Signatures, ported from a2x/cs2-dumper's src/analysis/offsets.rs so they stay
// traceable to a maintained source rather than being invented here.
// ---------------------------------------------------------------------------
struct GlobalSig {
    const char* name;
    ptrdiff_t*  target;
    const char* pattern;
    int         relOffset;
    int         instrLen;
};

const GlobalSig kGlobals[] = {
    // 48 89 0D rel32   mov [rip+x], rcx
    { "dwEntityList", &offsets::dwEntityList,
      "48 89 0D ? ? ? ? E9 ? ? ? ? CC", 3, 7 },

    // 48 8D 0D rel32   lea rcx, [rip+x]
    { "dwViewMatrix", &offsets::dwViewMatrix,
      "48 8D 0D ? ? ? ? 48 C1 E0 06", 3, 7 },

    // 48 8B 05 rel32   mov rax, [rip+x]
    { "dwLocalPlayerController", &offsets::dwLocalPlayerController,
      "48 8B 05 ? ? ? ? 41 89 BE", 3, 7 },

    // 48 89 0D rel32   mov [rip+x], rcx   - global vars, for the bomb timer
    { "dwGlobalVars", &offsets::dwGlobalVars,
      "48 89 0D ? ? ? ? 48 89 41 ? 48 8B 4C 24", 3, 7 },
};

// These two are reached in two steps: a base global, then a struct member
// offset read out of a nearby instruction.
struct DerivedSig {
    const char* name;
    ptrdiff_t*  target;
    const char* basePattern;
    int         baseRel;
    int         baseLen;
    const char* memberPattern;
    int         memberImmOffset;
};

const DerivedSig kDerived[] = {
    // dwPrediction, then `cmp [rsi+u32], r14` gives m_hLocalPawn's slot.
    { "dwLocalPlayerPawn", &offsets::dwLocalPlayerPawn,
      "48 8D 05 ? ? ? ? C3 CC CC CC CC CC CC CC CC 40 53 56 41 54", 3, 7,
      "4C 39 B6 ? ? ? ? 74 ? 44 88 BE", 3 },

    // dwCSGOInput, then `movss xmm0, [rax+rbp+u32]` gives the view angles slot.
    { "dwViewAngles", &offsets::dwViewAngles,
      "48 89 05 ? ? ? ? 0F 57 C0 0F 11 05", 3, 7,
      "F2 42 0F 10 84 28 ? ? ? ?", 6 },
};

struct FieldSig {
    const char* cls;
    const char* field;
    ptrdiff_t*  target;
};

const FieldSig kFields[] = {
    { "C_BaseEntity",          "m_iHealth",              &schema::m_iHealth },
    { "C_BaseEntity",          "m_iTeamNum",             &schema::m_iTeamNum },
    { "C_BaseEntity",          "m_pGameSceneNode",       &schema::m_pGameSceneNode },
    { "C_BaseEntity",          "m_lifeState",            &schema::m_lifeState },
    { "CGameSceneNode",        "m_vecAbsOrigin",         &schema::m_vecAbsOrigin },
    { "CBasePlayerController", "m_steamID",              &schema::m_steamID },
    { "CCSPlayerController",   "m_sSanitizedPlayerName", &schema::m_sSanitizedPlayerName },
    { "CCSPlayerController",   "m_hPlayerPawn",          &schema::m_hPlayerPawn },
    { "CCSPlayerController",   "m_bPawnIsAlive",         &schema::m_bPawnIsAlive },
    { "CCSPlayerController",   "m_iPawnHealth",          &schema::m_iPawnHealth },
    { "CGameSceneNode",        "m_bDormant",             &schema::m_bDormant },
    { "CSkeletonInstance",     "m_modelState",           &schema::m_modelState },
    { "C_BaseModelEntity",     "m_vecViewOffset",        &schema::m_vecViewOffset },
    { "C_CSPlayerPawn",        "m_angEyeAngles",         &schema::m_angEyeAngles },
    { "C_BaseEntity",          "m_vecVelocity",          &schema::m_vecVelocity },
    { "C_CSPlayerPawn",        "m_ArmorValue",           &schema::m_ArmorValue },
    { "C_CSPlayerPawn",        "m_bIsScoped",            &schema::m_bIsScoped },
    { "C_CSPlayerPawn",        "m_iShotsFired",          &schema::m_iShotsFired },
    { "C_CSPlayerPawn",        "m_aimPunchAngle",        &schema::m_aimPunchAngle },
    // m_entitySpottedState is declared on a base class in some dumps and on the
    // pawn in others. Both are tried; a miss keeps the baked value and only the
    // successful lookup writes, so the order is a fallback chain.
    { "C_BaseEntity",          "m_entitySpottedState",   &schema::m_entitySpottedState },
    { "C_CSPlayerPawnBase",    "m_entitySpottedState",   &schema::m_entitySpottedState },
    { "C_CSPlayerPawn",        "m_entitySpottedState",   &schema::m_entitySpottedState },
    { "C_PlantedC4",           "m_flC4Blow",             &schema::m_flC4Blow },
    { "C_PlantedC4",           "m_bBombTicking",         &schema::m_bBombTicking },
    { "C_PlantedC4",           "m_bBombDefused",         &schema::m_bBombDefused },
    { "C_PlantedC4",           "m_nBombSite",            &schema::m_nBombSite },
    { "C_PlantedC4",           "m_bBeingDefused",        &schema::m_bBeingDefused },
    { "C_PlantedC4",           "m_flDefuseCountDown",    &schema::m_flDefuseCountDown },
    { "C_PlantedC4",           "m_flTimerLength",        &schema::m_flTimerLength },
    { "C_PlantedC4",           "m_flDefuseLength",       &schema::m_flDefuseLength },
    { "C_PlantedC4",           "m_hBombDefuser",         &schema::m_hBombDefuser },
    { "C_BaseEntity",          "m_hOwnerEntity",         &schema::m_hOwnerEntity },
    { "C_BasePlayerWeapon",    "m_iClip1",               &schema::m_iClip1 },
    { "C_BasePlayerWeapon",    "m_nNextPrimaryAttackTick", &schema::m_nNextPrimaryAttackTick },
    { "EntitySpottedState_t",  "m_bSpottedByMask",       &schema::m_bSpottedByMask },
    { "C_BaseEntity",          "m_fFlags",               &schema::m_fFlags },
    { "C_BaseEntity",          "m_MoveType",             &schema::m_MoveType },
    { "C_BasePlayerPawn",      "m_pWeaponServices",      &schema::m_pWeaponServices },
    { "C_BasePlayerPawn",      "m_pItemServices",        &schema::m_pItemServices },
    { "CPlayer_WeaponServices","m_hActiveWeapon",        &schema::m_hActiveWeapon },
    { "CCSPlayer_ItemServices","m_bHasHelmet",           &schema::m_bHasHelmet },
    { "C_EconEntity",          "m_AttributeManager",     &schema::m_AttributeManager },
    { "C_AttributeContainer",  "m_Item",                 &schema::m_Item },
    { "C_EconItemView",        "m_iItemDefinitionIndex", &schema::m_iItemDefinitionIndex },
    { "C_CSWeaponBase",        "m_fAccuracyPenalty",     &schema::m_fAccuracyPenalty },
    { "C_CSWeaponBase",        "m_bInReload",            &schema::m_bInReload },
    { "C_BaseModelEntity",     "m_Glow",                 &schema::m_Glow },
    { "CGlowProperty",         "m_iGlowType",            &schema::m_iGlowType },
    { "CGlowProperty",         "m_nGlowRange",           &schema::m_nGlowRange },
    { "CGlowProperty",         "m_glowColorOverride",    &schema::m_glowColorOverride },
    { "CGlowProperty",         "m_bFlashing",            &schema::m_bFlashing },
    { "CGlowProperty",         "m_bGlowing",             &schema::m_bGlowing },
    { "CGlowProperty",         "m_fGlowColor",           &schema::m_fGlowColor },
    { "CGlowProperty",         "m_iGlowTeam",            &schema::m_iGlowTeam },
    { "CGlowProperty",         "m_nGlowRangeMin",        &schema::m_nGlowRangeMin },
    { "CGlowProperty",         "m_flGlowTime",           &schema::m_flGlowTime },
    { "CGlowProperty",         "m_flGlowStartTime",      &schema::m_flGlowStartTime },
    { "CGlowProperty",         "m_bEligibleForScreenHighlight",
                                                         &schema::m_bEligibleForScreenHighlight },
};

// Logs the change and reports whether the value moved.
bool Apply(const char* what, ptrdiff_t* slot, ptrdiff_t found) {
    const ptrdiff_t baked = *slot;
    *slot = found;

    if (found == baked) {
        DBG_OK("resolve", "%-24s 0x%llX (matches baked)", what,
               static_cast<unsigned long long>(found));
        return false;
    }

    DBG_W("resolve", "%-24s 0x%llX  (baked was 0x%llX - CORRECTED)", what,
          static_cast<unsigned long long>(found),
          static_cast<unsigned long long>(baked));
    return true;
}

} // namespace

namespace resolve {

Result Run() {
    Result r;
    // Totals are the size of the tables, so a skipped stage still reads as
    // "0 of 10" rather than a flattering "0 of 0".
    r.globalsTotal = static_cast<int>(sizeof(kGlobals) / sizeof(kGlobals[0]) +
                                      sizeof(kDerived) / sizeof(kDerived[0]));
    r.fieldsTotal = static_cast<int>(sizeof(kFields) / sizeof(kFields[0]));

    DBG_I("resolve", "--- resolving offsets from the running game ---");

    // ---- client.dll globals via signature scan ----------------------------
    const sig::Region client = sig::Executable("client.dll");
    if (!client.base) {
        DBG_E("resolve", "client.dll has no executable section - keeping baked globals");
    } else {
        DBG_I("resolve", "scanning client.dll .text at 0x%llX (%zu bytes)",
              static_cast<unsigned long long>(client.start), client.size);

        for (const GlobalSig& s : kGlobals) {
            const uintptr_t hit = sig::Find(client, s.pattern);
            const uintptr_t target = sig::RipTarget(hit, s.relOffset, s.instrLen);
            if (!hit || !target) {
                DBG_E("resolve", "%-24s pattern not found - keeping baked 0x%llX", s.name,
                      static_cast<unsigned long long>(*s.target));
                continue;
            }
            ++r.globalsFound;
            r.anyDisagreement |= Apply(s.name, s.target,
                                       static_cast<ptrdiff_t>(target - client.base));
        }

        for (const DerivedSig& s : kDerived) {
            const uintptr_t baseHit = sig::Find(client, s.basePattern);
            const uintptr_t baseTarget = sig::RipTarget(baseHit, s.baseRel, s.baseLen);
            const uintptr_t memberHit = sig::Find(client, s.memberPattern);
            const uint32_t member = sig::Imm32(memberHit, s.memberImmOffset);

            if (!baseTarget || !memberHit) {
                DBG_E("resolve", "%-24s pattern not found - keeping baked 0x%llX", s.name,
                      static_cast<unsigned long long>(*s.target));
                continue;
            }
            ++r.globalsFound;
            r.anyDisagreement |= Apply(s.name, s.target,
                                       static_cast<ptrdiff_t>(baseTarget - client.base) + member);
        }
    }

    // ---- schema fields, straight from the game's own schema system --------
    const uintptr_t scope = schemasys::TypeScope("client.dll");
    if (!scope) {
        DBG_E("resolve", "no client.dll type scope - keeping baked field offsets");
    } else {
        r.schemaClasses = schemasys::ClassCount(scope);
        DBG_I("resolve", "%d classes in the client.dll scope", r.schemaClasses);

        if (r.schemaClasses < 100) {
            DBG_E("resolve", "suspiciously few classes - the UtlTsHash layout may have moved");
        } else {
            r.schemaOk = true;
        }

        for (const FieldSig& f : kFields) {
            const ptrdiff_t off = schemasys::FieldOffset(scope, f.cls, f.field);
            if (off < 0) {
                DBG_E("resolve", "%s::%s not found - keeping baked 0x%llX",
                      f.cls, f.field, static_cast<unsigned long long>(*f.target));
                continue;
            }
            ++r.fieldsFound;

            char label[96];
            snprintf(label, sizeof(label), "%s::%s", f.cls, f.field);
            r.anyDisagreement |= Apply(label, f.target, off);
        }
    }

    dbg::Print(r.fieldsFound == r.fieldsTotal && r.globalsFound == r.globalsTotal
                   ? dbg::Level::Ok : dbg::Level::Warn,
               "resolve", "globals %d/%d, fields %d/%d resolved",
               r.globalsFound, r.globalsTotal, r.fieldsFound, r.fieldsTotal);

    if (r.anyDisagreement)
        DBG_W("resolve", "baked offsets were stale - runtime values are now in use");

    g_last = r;
    return r;
}

const Result& Last() { return g_last; }

bool DumpToFile() {
    char path[MAX_PATH]{};
    char temp[MAX_PATH]{};
    if (!GetTempPathA(MAX_PATH, temp))
        return false;
    snprintf(path, sizeof(path), "%scs2_overlay_offsets.txt", temp);

    FILE* f = nullptr;
    if (fopen_s(&f, path, "w") != 0 || !f) {
        DBG_E("resolve", "cannot write %s", path);
        return false;
    }

    fprintf(f, "; cs2_overlay - offsets read from the running game\n");
    fprintf(f, "; baked dump build: %d\n\n", offsets::kDumpBuild);

    fprintf(f, "[client.dll globals]\n");
    for (const GlobalSig& s : kGlobals)
        fprintf(f, "  %-26s 0x%llX\n", s.name, static_cast<unsigned long long>(*s.target));
    for (const DerivedSig& s : kDerived)
        fprintf(f, "  %-26s 0x%llX\n", s.name, static_cast<unsigned long long>(*s.target));

    fprintf(f, "\n[schema fields]\n");
    for (const FieldSig& s : kFields) {
        char label[96];
        snprintf(label, sizeof(label), "%s::%s", s.cls, s.field);
        fprintf(f, "  %-46s 0x%llX\n", label, static_cast<unsigned long long>(*s.target));
    }
    fclose(f);

    DBG_OK("resolve", "wrote %s", path);

    // The full class dump lands next to it.
    char schemaPath[MAX_PATH]{};
    snprintf(schemaPath, sizeof(schemaPath), "%scs2_overlay_schema.txt", temp);

    const char* classes[] = {
        "C_BaseEntity", "CGameSceneNode", "CBasePlayerController",
        "CCSPlayerController", "C_CSPlayerPawn", "C_CSPlayerPawnBase",
        "C_CSWeaponBase", "CPlayer_WeaponServices", "CCSPlayer_ItemServices",
        "C_AttributeContainer", "C_EconItemView",
    };
    const uintptr_t scope = schemasys::TypeScope("client.dll");
    if (scope)
        schemasys::DumpClasses(scope, classes,
                               static_cast<int>(sizeof(classes) / sizeof(classes[0])),
                               schemaPath);

    return true;
}

} // namespace resolve
