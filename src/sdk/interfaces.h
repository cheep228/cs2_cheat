#pragma once
#include <cstdint>

// The game singletons this project talks to.
//
// Two different mechanisms, because Source 2 uses two: interfaces that a
// module exports through CreateInterface, and singletons that only exist as a
// pointer inside some function, reachable by pattern.
//
// Every field here is allowed to be null. Resolution runs on a live game and
// a game update can move any of it, so the rule throughout is: check, log,
// and leave the dependent feature off. Nothing dereferences these blind.
namespace ifaces {

struct GlobalVars {
    float realTime;          // 0x00
    int   frameCount;        // 0x04
    float frameTime;         // 0x08
    float absFrameTime;      // 0x0C
    int   maxClients;        // 0x10
    float intervalPerTick;   // 0x14
    char  pad0[0x14];
    float curTime;           // 0x2C
    float curTime2;          // 0x30
    char  pad1[0x0C];
    int   tickCount;         // 0x40
};

struct Set {
    // via CreateInterface
    uintptr_t schemaSystem   = 0;   // SchemaSystem_001
    uintptr_t engineClient   = 0;   // Source2EngineToClient001
    uintptr_t source2Client  = 0;   // Source2Client002
    uintptr_t engineCvar     = 0;   // VEngineCvar007
    uintptr_t inputSystem    = 0;   // InputSystemVersion001
    uintptr_t sceneSystem    = 0;   // SceneSystem_002
    uintptr_t materialSystem = 0;   // VMaterialSystem2_001

    // via pattern
    uintptr_t csgoInput      = 0;   // CCSGOInput, owner of CreateMove
    uintptr_t entitySystem   = 0;   // CGameEntitySystem
    uintptr_t globalVars     = 0;   // CGlobalVarsBase

    // Set when globalVars passed the plausibility check below.
    bool globalVarsTrusted = false;
};

extern Set set;

// Resolves everything it can and logs one line per item. Returns true when
// the pieces the internal hooks need (csgoInput, source2Client) are present.
// Safe to call repeatedly - already-resolved entries are left alone.
bool Resolve();

// Live global vars, or null when they were never resolved or failed the
// plausibility check (a wrong offset here poisons every timing decision, so
// an untrusted pointer is treated as no pointer at all).
const GlobalVars* Globals();

// True once Resolve() found enough to install the game hooks.
bool ReadyForHooks();

} // namespace ifaces
