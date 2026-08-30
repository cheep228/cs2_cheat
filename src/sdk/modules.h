#pragma once
#include <cstdint>

#include "sdk/sigscan.h"

// The game modules this project reaches into, resolved once and cached.
//
// Every one of them is optional: a module that is not loaded yields an empty
// region and its dependent features stay off rather than taking the process
// down. Nothing here assumes CS2 has finished loading - Ready() says whether
// the essential ones turned up.
namespace modules {

struct Set {
    sig::Region client;             // client.dll
    sig::Region engine2;            // engine2.dll
    sig::Region schemasystem;       // schemasystem.dll
    sig::Region scenesystem;        // scenesystem.dll
    sig::Region rendersystem;       // rendersystemdx11.dll
    sig::Region materialsystem2;    // materialsystem2.dll
    sig::Region vphysics2;          // vphysics2.dll
    sig::Region inputsystem;        // inputsystem.dll
    sig::Region particles;          // particles.dll
    sig::Region tier0;              // tier0.dll
};

extern Set set;

// Resolves every module and logs one line each. Safe to call repeatedly; a
// module that was missing last time is retried.
void Refresh();

// client.dll and engine2.dll are present - the bare minimum for anything.
bool Ready();

// Convenience wrapper: pattern search inside one module's .text.
uintptr_t Find(const sig::Region& region, const char* pattern, const char* what);

// Pattern search that then resolves a RIP-relative operand and dereferences
// it once, which is how Source 2 exposes most of its singletons:
//     mov rcx, [rip+disp]   ->   the interface pointer
uintptr_t FindGlobalPtr(const sig::Region& region, const char* pattern,
                        int relOffset, int instrLen, const char* what);

} // namespace modules
