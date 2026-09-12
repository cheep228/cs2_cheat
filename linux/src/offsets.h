#pragma once
#include <cstdint>

// ---------------------------------------------------------------------------
// The only numbers in this whole project that are NOT resolved from the
// live game. Everything else (schema field offsets, interface pointers) is
// found at runtime by name - see schema_system.cpp and fields.cpp - which is
// why this project does not ship a baked offset dump the way the Windows
// build's sdk/offsets.h does.
//
// These three cannot be found that way: they are raw static pointers inside
// libclient.so (entity list, local player pawn, local player controller),
// reachable only by recognising the specific machine code the Linux build's
// compiler emitted around them. I do not have the real CS2 Linux binary to
// scan, so I am not going to invent byte patterns or addresses for it - a
// wrong guess here does not fail loudly, it silently reads or writes the
// wrong memory, which is exactly how you corrupt game state or crash it.
//
// Fill these in yourself with values taken from YOUR OWN copy of the game's
// libclient.so (see linux/README.md, "Getting the three globals"). All three
// are offsets FROM libclient.so's base address, same convention as the
// Windows project's offsets::dwEntityList etc.
// ---------------------------------------------------------------------------

namespace offsets {

inline uintptr_t dwEntityList            = 0;   // TODO: fill in, see README
inline uintptr_t dwLocalPlayerPawn       = 0;   // TODO: fill in, see README
inline uintptr_t dwLocalPlayerController = 0;   // TODO: fill in, see README

inline bool Ready() {
    return dwEntityList != 0 && dwLocalPlayerPawn != 0;
}

} // namespace offsets
